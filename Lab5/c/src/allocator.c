#include "allocator.h"

#include "buddy.h"
#include "daif.h"
#include "list.h"
#include "log.h"
#include "util.h"

/* Private constants */

#define DYNAMIC_POOL_COUNT (2 + 2 * (CONFIG_BUDDY_PAGE_SHIFT - 6))

/* Private types */

typedef enum {
    DYNAMIC_BLOCK_SMALL = 1,
    DYNAMIC_BLOCK_LARGE,
} dynamic_block_type_t;

typedef struct dynamic_free_chunk {
    struct dynamic_free_chunk *next;
} dynamic_free_chunk_t;

typedef struct {
    uint32_t type;
    uint32_t pool_index;
    list_head_t anchor;
    dynamic_free_chunk_t *first_free;
    uint16_t chunk_count;
    uint16_t free_count;
} dynamic_pool_page_t;

typedef struct {
    uint32_t type;
    uint32_t reserved;
    size_t page_count;
} dynamic_large_block_t;

typedef struct {
    list_head_t available_pages;
    size_t chunk_size;
} dynamic_pool_t;

typedef struct {
    bool ready;
    dynamic_pool_t pools[DYNAMIC_POOL_COUNT];
} dynamic_allocator_t;

/* Private function declarations */

/** Initialize size-class pools after the buddy allocator is ready. */
static bool dynamic_allocator_init(void);

/** Return whether the dynamic allocator has completed initialization. */
static bool dynamic_allocator_is_ready(void);

/** Allocate from a size-class pool or a large block, protecting shared state from IRQ. */
static void *dynamic_malloc(size_t size);

/** Dispatch a free by block type, protecting shared state from IRQ. */
static void dynamic_free(void *address);

/** Find the smallest fitting pool, or return DYNAMIC_POOL_COUNT for a large request. */
static uint32_t find_pool_index(size_t size);

/** Obtain a buddy page and divide it into free chunks for the selected pool. */
static dynamic_pool_page_t *create_pool_page(uint32_t pool_index);

/** Take a free chunk from the selected pool, creating a backing page if needed. */
static void *allocate_small(uint32_t pool_index);

/** Allocate contiguous pages for the header and payload, returning the payload address. */
static void *allocate_large(size_t size);

/** Return a chunk to its pool and release the backing page when all chunks are free. */
static void free_small(dynamic_pool_page_t *page, void *address);

/** Validate the payload address and return the large block's pages to buddy. */
static void free_large(dynamic_large_block_t *block, void *address);

/* Private data */

extern char simple_heap_begin;
extern char simple_heap_end;

static uintptr_t heap_begin;
static uintptr_t heap_current;
static uintptr_t heap_end;

static dynamic_allocator_t dynamic_allocator;

_Static_assert((sizeof(dynamic_pool_page_t) + KERNEL_ALLOC_ALIGNMENT - 1) / KERNEL_ALLOC_ALIGNMENT *
                       KERNEL_ALLOC_ALIGNMENT <=
                   BUDDY_PAGE_SIZE / 4,
               "dynamic pool page header must fit in one quarter of a buddy page");
_Static_assert(sizeof(dynamic_free_chunk_t) <= KERNEL_ALLOC_ALIGNMENT,
               "the smallest chunk must hold a free-list pointer");

/* Function implementations */

void simple_allocator_init(void)
{
    heap_begin = (uintptr_t) &simple_heap_begin;
    heap_current = heap_begin;
    heap_end = (uintptr_t) &simple_heap_end;
}

void *simple_malloc(size_t size)
{
    uintptr_t allocated;
    uintptr_t next;

    if (size == 0) {
        return NULL;
    }

    allocated = heap_current;
    next = align_up(allocated + size, KERNEL_ALLOC_ALIGNMENT);

    if (next < allocated || next > heap_end) {
        return NULL;
    }

    heap_current = next;
    return (void *) allocated;
}

uintptr_t simple_allocator_begin(void)
{
    return heap_begin;
}

uintptr_t simple_allocator_current(void)
{
    return heap_current;
}

uintptr_t simple_allocator_end(void)
{
    return heap_end;
}

size_t simple_allocator_remaining(void)
{
    if (heap_current > heap_end) {
        return 0;
    }

    return (size_t) (heap_end - heap_current);
}

bool kernel_allocator_init(void)
{
    return dynamic_allocator_init();
}

bool kernel_allocator_is_ready(void)
{
    return dynamic_allocator_is_ready();
}

void *kernel_malloc(size_t size)
{
    if (dynamic_allocator_is_ready()) {
        return dynamic_malloc(size);
    }

    return simple_malloc(size);
}

void kernel_free(void *address)
{
    uintptr_t value = (uintptr_t) address;

    if (address == NULL) {
        return;
    }

    if (value >= heap_begin && value < heap_end) {
        LOG_VERBOSE("allocator", "ignore free of startup heap address 0x%08X", (uint32_t) value);
        return;
    }

    if (!dynamic_allocator_is_ready()) {
        LOG_VERBOSE("allocator", "ignore free before dynamic initialization");
        return;
    }

    dynamic_free(address);
}

static bool dynamic_allocator_init(void)
{
    uint32_t pool_index = 0;
    size_t chunk_size;

    if (dynamic_allocator.ready || buddy_get_state() != BUDDY_STATE_READY) {
        return false;
    }

    LIST_INIT(&dynamic_allocator.pools[pool_index].available_pages);
    dynamic_allocator.pools[pool_index++].chunk_size = KERNEL_ALLOC_ALIGNMENT;

    LIST_INIT(&dynamic_allocator.pools[pool_index].available_pages);
    dynamic_allocator.pools[pool_index++].chunk_size = KERNEL_ALLOC_ALIGNMENT << 1;

    for (chunk_size = KERNEL_ALLOC_ALIGNMENT << 2; chunk_size <= BUDDY_PAGE_SIZE / 4;
         chunk_size <<= 1) {
        LIST_INIT(&dynamic_allocator.pools[pool_index].available_pages);
        dynamic_allocator.pools[pool_index++].chunk_size = chunk_size;

        LIST_INIT(&dynamic_allocator.pools[pool_index].available_pages);
        dynamic_allocator.pools[pool_index++].chunk_size = chunk_size + chunk_size / 2;
    }

    if (pool_index != DYNAMIC_POOL_COUNT) {
        return false;
    }

    dynamic_allocator.ready = true;
    return true;
}

static bool dynamic_allocator_is_ready(void)
{
    return dynamic_allocator.ready;
}

static void *dynamic_malloc(size_t size)
{
    uint32_t pool_index;
    daif_irq_state_t irq_state;
    void *address;

    if (!dynamic_allocator.ready || size == 0) {
        return NULL;
    }

    pool_index = find_pool_index(size);
    irq_state = daif_irq_save();
    if (pool_index < DYNAMIC_POOL_COUNT) {
        address = allocate_small(pool_index);
    } else {
        address = allocate_large(size);
    }
    daif_irq_restore(irq_state);

    LOG_VERBOSE("allocator", "allocate %u bytes at 0x%08X", (uint32_t) size,
                (uint32_t) (uintptr_t) address);
    return address;
}

static void dynamic_free(void *address)
{
    uintptr_t page_address;
    uint32_t type;
    daif_irq_state_t irq_state;

    if (!dynamic_allocator.ready || address == NULL) {
        return;
    }

    irq_state = daif_irq_save();
    page_address = (uintptr_t) address & ~(BUDDY_PAGE_SIZE - 1);
    type = *(uint32_t *) page_address;

    if (type == DYNAMIC_BLOCK_SMALL) {
        free_small((dynamic_pool_page_t *) page_address, address);
    } else if (type == DYNAMIC_BLOCK_LARGE) {
        free_large((dynamic_large_block_t *) page_address, address);
    } else {
        LOG_VERBOSE("allocator", "ignore invalid free at 0x%08X", (uint32_t) (uintptr_t) address);
    }
    daif_irq_restore(irq_state);
}

static uint32_t find_pool_index(size_t size)
{
    for (uint32_t i = 0; i < DYNAMIC_POOL_COUNT; i++) {
        if (size <= dynamic_allocator.pools[i].chunk_size) {
            return i;
        }
    }

    return DYNAMIC_POOL_COUNT;
}

static dynamic_pool_page_t *create_pool_page(uint32_t pool_index)
{
    dynamic_pool_t *pool = &dynamic_allocator.pools[pool_index];
    dynamic_pool_page_t *page;
    uintptr_t chunks_begin;
    size_t header_size;
    size_t available_size;
    size_t chunk_count;
    void *address;

    if (buddy_alloc_pages(1, &address) != BUDDY_SUCCESS) {
        return NULL;
    }

    page = address;
    header_size = align_up(sizeof(*page), KERNEL_ALLOC_ALIGNMENT);
    chunks_begin = (uintptr_t) page + header_size;
    available_size = BUDDY_PAGE_SIZE - header_size;
    chunk_count = available_size / pool->chunk_size;
    if (chunk_count == 0 || chunk_count > UINT16_MAX) {
        buddy_free_pages(page, 1);
        return NULL;
    }

    page->type = DYNAMIC_BLOCK_SMALL;
    page->pool_index = pool_index;
    LIST_INIT(&page->anchor);
    page->first_free = NULL;
    page->chunk_count = (uint16_t) chunk_count;
    page->free_count = (uint16_t) chunk_count;

    for (size_t i = 0; i < chunk_count; i++) {
        dynamic_free_chunk_t *chunk =
            (dynamic_free_chunk_t *) (chunks_begin + i * pool->chunk_size);

        chunk->next = page->first_free;
        page->first_free = chunk;
    }

    list_add_last(&page->anchor, &pool->available_pages);
    LOG_VERBOSE("allocator", "create %u-byte pool page at 0x%08X with %u chunks",
                (uint32_t) pool->chunk_size, (uint32_t) (uintptr_t) page,
                (uint32_t) page->chunk_count);
    return page;
}

static void *allocate_small(uint32_t pool_index)
{
    dynamic_pool_t *pool = &dynamic_allocator.pools[pool_index];
    dynamic_pool_page_t *page;
    dynamic_free_chunk_t *chunk;

    if (list_is_empty(&pool->available_pages)) {
        page = create_pool_page(pool_index);
        if (page == NULL) {
            return NULL;
        }
    } else {
        page = container_of(pool->available_pages.next, dynamic_pool_page_t, anchor);
    }

    chunk = page->first_free;
    page->first_free = chunk->next;
    page->free_count--;
    if (page->free_count == 0) {
        list_remove(&page->anchor);
    }

    return chunk;
}

static void *allocate_large(size_t size)
{
    const size_t header_size = align_up(sizeof(dynamic_large_block_t), KERNEL_ALLOC_ALIGNMENT);
    dynamic_large_block_t *block;
    size_t required_size;
    size_t page_count;
    void *address;

    if (size > (size_t) -1 - header_size) {
        return NULL;
    }
    required_size = header_size + size;
    page_count = ((required_size - 1) >> CONFIG_BUDDY_PAGE_SHIFT) + 1;
    if (buddy_alloc_pages(page_count, &address) != BUDDY_SUCCESS) {
        return NULL;
    }

    block = address;
    block->type = DYNAMIC_BLOCK_LARGE;
    block->reserved = 0;
    block->page_count = page_count;
    LOG_VERBOSE("allocator", "create %u-page large block at 0x%08X", (uint32_t) page_count,
                (uint32_t) (uintptr_t) block);
    return (void *) ((uintptr_t) block + header_size);
}

static void free_small(dynamic_pool_page_t *page, void *address)
{
    dynamic_pool_t *pool;
    dynamic_free_chunk_t *chunk;
    uintptr_t chunks_begin;
    uintptr_t chunk_offset;
    bool was_full;

    if (page->pool_index >= DYNAMIC_POOL_COUNT) {
        LOG_VERBOSE("allocator", "invalid pool index at 0x%08X", (uint32_t) (uintptr_t) address);
        return;
    }

    pool = &dynamic_allocator.pools[page->pool_index];
    chunks_begin = align_up((uintptr_t) page + sizeof(*page), KERNEL_ALLOC_ALIGNMENT);
    if ((uintptr_t) address < chunks_begin) {
        LOG_VERBOSE("allocator", "invalid small allocation at 0x%08X",
                    (uint32_t) (uintptr_t) address);
        return;
    }

    chunk_offset = (uintptr_t) address - chunks_begin;
    if (chunk_offset >= (size_t) page->chunk_count * pool->chunk_size ||
        chunk_offset % pool->chunk_size != 0 || page->free_count >= page->chunk_count) {
        LOG_VERBOSE("allocator", "invalid small allocation at 0x%08X",
                    (uint32_t) (uintptr_t) address);
        return;
    }

    was_full = page->free_count == 0;
    chunk = address;
    chunk->next = page->first_free;
    page->first_free = chunk;
    page->free_count++;

    if (was_full) {
        list_add_last(&page->anchor, &pool->available_pages);
    }

    if (page->free_count == page->chunk_count) {
        buddy_error_t error;

        list_remove(&page->anchor);
        error = buddy_free_pages(page, 1);
        if (error != BUDDY_SUCCESS) {
            list_add_last(&page->anchor, &pool->available_pages);
            LOG_VERBOSE("allocator", "failed to release pool page 0x%08X: %s",
                        (uint32_t) (uintptr_t) page, buddy_error_string(error));
            return;
        }

        LOG_VERBOSE("allocator", "release empty pool page 0x%08X", (uint32_t) (uintptr_t) page);
    }
}

static void free_large(dynamic_large_block_t *block, void *address)
{
    const size_t header_size = align_up(sizeof(*block), KERNEL_ALLOC_ALIGNMENT);
    size_t page_count;
    buddy_error_t error;

    if ((uintptr_t) address != (uintptr_t) block + header_size || block->page_count == 0) {
        LOG_VERBOSE("allocator", "invalid large allocation at 0x%08X",
                    (uint32_t) (uintptr_t) address);
        return;
    }

    page_count = block->page_count;
    error = buddy_free_pages(block, page_count);
    if (error != BUDDY_SUCCESS) {
        LOG_VERBOSE("allocator", "failed to release large block 0x%08X: %s",
                    (uint32_t) (uintptr_t) block, buddy_error_string(error));
        return;
    }

    LOG_VERBOSE("allocator", "release %u-page large block 0x%08X", (uint32_t) page_count,
                (uint32_t) (uintptr_t) block);
}
