#include "buddy.h"

#include "allocator.h"
#include "config.h"
#include "list.h"
#include "log.h"
#include "printf.h"

/* Private types */

typedef enum {
    BUDDY_PAGE_FREE = 0,
    BUDDY_PAGE_ALLOCATED,
    BUDDY_PAGE_TAIL,
    BUDDY_PAGE_RESERVED,
} buddy_page_state_t;

typedef struct {
    /* Two state bits and six order bits keep metadata at one byte per physical page. */
    uint8_t state : 2;
    uint8_t order : 6;
} buddy_page_t;

typedef struct {
    list_head_t free_list;
    size_t free_count;
} buddy_free_area_t;

typedef struct {
    buddy_state_t state;
    uintptr_t memory_base;
    uintptr_t memory_end;
    size_t page_count;
    buddy_page_t *pages;
    buddy_free_area_t free_areas[BUDDY_ORDER_COUNT];
} buddy_system_t;

_Static_assert(sizeof(buddy_page_t) == 1, "buddy page metadata must occupy one byte");
_Static_assert(CONFIG_BUDDY_MAX_ORDER <= 63,
               "buddy maximum order must fit in the fixed 6-bit metadata field");

/* Private function declarations */

/** Convert a managed page index to its physical base address. */
static uintptr_t page_index_to_address(size_t page_index);

/** Convert a physical address to its page index relative to the managed range. */
static size_t address_to_page_index(uintptr_t address);

/** Locate the free-list node stored at the beginning of a page. */
static list_head_t *page_index_to_node(size_t page_index);

/** Recover the page index from an embedded free-list node address. */
static size_t node_to_page_index(const list_head_t *node);

/** Append a block to its order's free list and increment the free-block count. */
static void add_free_block(size_t page_index);

/** Unlink a block from its order's free list and decrement the free-block count. */
static void remove_free_block(size_t page_index);

/** Find the smallest order that can hold the requested page count. */
static uint32_t page_count_to_order(size_t page_count);

/** Allocate a block of the requested order, splitting a larger block if necessary. */
static buddy_error_t allocate_order(uint32_t order, size_t *page_index);

/** Split an unlisted free block down to target_order, returning unused halves to free lists. */
static void split_free_block(size_t page_index, uint32_t target_order);

/** Keep the requested allocated prefix and return excess suffix pages to free lists. */
static void trim_allocated_block(size_t page_index, size_t page_count);

/** Free one order-sized block and repeatedly merge it with available buddies. */
static void free_order_block(size_t page_index);

/** Log an operation with the block address and order when verbose logging is enabled. */
static void log_block(const char *operation, size_t page_index);

/* Private data */

static buddy_system_t buddy_system;

static const char *const buddy_error_messages[] = {
    "Success.",
    "Buddy system is already initialized.",
    "Invalid managed memory range.",
    "Managed memory range is not page aligned.",
    "Unable to allocate buddy metadata.",
    "Buddy system is not accepting reservations.",
    "Buddy system is not ready.",
    "Page request exceeds the largest buddy block.",
    "Buddy system does not contain a large enough contiguous block.",
    "Invalid argument.",
    "Address is outside managed memory.",
    "Address is not page aligned.",
    "Address is not an allocated block head.",
};

#define BUDDY_ERROR_COUNT (sizeof(buddy_error_messages) / sizeof(buddy_error_messages[0]))

/* Function implementations */

const char *buddy_error_string(buddy_error_t error)
{
    size_t index;

    if (error > BUDDY_SUCCESS) {
        return "Unknown buddy allocator result.";
    }

    index = (size_t) -error;
    if (index >= BUDDY_ERROR_COUNT) {
        return "Unknown buddy allocator error.";
    }

    return buddy_error_messages[index];
}

buddy_error_t buddy_init(uintptr_t memory_base, uintptr_t memory_end)
{
    size_t page_count;
    buddy_page_t *pages;

    if (buddy_system.state != BUDDY_STATE_UNINITIALIZED) {
        return BUDDY_ERROR_ALREADY_INITIALIZED;
    }
    if (memory_end <= memory_base) {
        return BUDDY_ERROR_INVALID_RANGE;
    }
    if ((memory_base & (BUDDY_PAGE_SIZE - 1)) != 0 ||
        (memory_end & (BUDDY_PAGE_SIZE - 1)) != 0) {
        return BUDDY_ERROR_UNALIGNED_RANGE;
    }

    page_count = (memory_end - memory_base) >> CONFIG_BUDDY_PAGE_SHIFT;
    pages = simple_malloc(page_count * sizeof(*pages));
    if (pages == NULL) {
        return BUDDY_ERROR_METADATA_ALLOCATION_FAILED;
    }

    buddy_system.memory_base = memory_base;
    buddy_system.memory_end = memory_end;
    buddy_system.page_count = page_count;
    buddy_system.pages = pages;

    for (size_t i = 0; i < page_count; i++) {
        pages[i].state = BUDDY_PAGE_FREE;
        pages[i].order = 0;
    }
    for (uint32_t order = 0; order < BUDDY_ORDER_COUNT; order++) {
        LIST_INIT(&buddy_system.free_areas[order].free_list);
        buddy_system.free_areas[order].free_count = 0;
    }

    buddy_system.state = BUDDY_STATE_RESERVING;
    return BUDDY_SUCCESS;
}

buddy_error_t buddy_reserve(uintptr_t begin, uintptr_t end)
{
    size_t first_page;
    size_t last_page;

    if (buddy_system.state != BUDDY_STATE_RESERVING) {
        return BUDDY_ERROR_NOT_RESERVING;
    }
    if (end <= begin || begin < buddy_system.memory_base || end > buddy_system.memory_end) {
        return BUDDY_ERROR_INVALID_RANGE;
    }

    first_page = (begin - buddy_system.memory_base) >> CONFIG_BUDDY_PAGE_SHIFT;
    last_page = (end - 1 - buddy_system.memory_base) >> CONFIG_BUDDY_PAGE_SHIFT;

    for (size_t page_index = first_page; page_index <= last_page; page_index++) {
        buddy_system.pages[page_index].state = BUDDY_PAGE_RESERVED;
    }

    LOG_VERBOSE("buddy", "reserve [0x%08X, 0x%08X)",
                (uint32_t) begin, (uint32_t) end);
    return BUDDY_SUCCESS;
}

buddy_error_t buddy_build(void)
{
    if (buddy_system.state != BUDDY_STATE_RESERVING) {
        return BUDDY_ERROR_NOT_RESERVING;
    }

    for (uint32_t order = 0; order < CONFIG_BUDDY_MAX_ORDER; order++) {
        size_t step = 1UL << (order + 1);
        size_t buddy_offset = 1UL << order;

        for (size_t page_index = 0; page_index < buddy_system.page_count; page_index += step) {
            size_t buddy_index = page_index + buddy_offset;

            if (buddy_index >= buddy_system.page_count) {
                continue;
            }
            if (buddy_system.pages[page_index].state != BUDDY_PAGE_FREE ||
                buddy_system.pages[buddy_index].state != BUDDY_PAGE_FREE ||
                buddy_system.pages[page_index].order != order ||
                buddy_system.pages[buddy_index].order != order) {
                continue;
            }

            buddy_system.pages[page_index].order = order + 1;
            buddy_system.pages[buddy_index].state = BUDDY_PAGE_TAIL;
        }
    }

    for (size_t page_index = 0; page_index < buddy_system.page_count; page_index++) {
        if (buddy_system.pages[page_index].state == BUDDY_PAGE_FREE) {
            add_free_block(page_index);
        }
    }

    buddy_system.state = BUDDY_STATE_READY;
    return BUDDY_SUCCESS;
}

buddy_error_t buddy_alloc_pages(size_t page_count, void **address)
{
    uint32_t order;
    size_t page_index;
    buddy_error_t error;

    if (buddy_system.state != BUDDY_STATE_READY) {
        return BUDDY_ERROR_NOT_READY;
    }
    if (address == NULL) {
        return BUDDY_ERROR_INVALID_ARGUMENT;
    }
    *address = NULL;
    if (page_count == 0) {
        return BUDDY_ERROR_INVALID_ARGUMENT;
    }
    if (page_count > (1UL << CONFIG_BUDDY_MAX_ORDER)) {
        return BUDDY_ERROR_REQUEST_TOO_LARGE;
    }

    order = page_count_to_order(page_count);
    error = allocate_order(order, &page_index);
    if (error != BUDDY_SUCCESS) {
        return error;
    }

    trim_allocated_block(page_index, page_count);
    *address = (void *) page_index_to_address(page_index);
    LOG_VERBOSE("buddy", "allocate 0x%08X, %u pages",
                (uint32_t) page_index_to_address(page_index),
                (uint32_t) page_count);
    return BUDDY_SUCCESS;
}

buddy_error_t buddy_free_pages(void *address, size_t page_count)
{
    uintptr_t block_address = (uintptr_t) address;
    size_t page_index;
    size_t current_index;
    size_t remaining_pages;

    if (buddy_system.state != BUDDY_STATE_READY) {
        return BUDDY_ERROR_NOT_READY;
    }
    if (page_count == 0) {
        return BUDDY_ERROR_INVALID_ARGUMENT;
    }
    if (page_count > (1UL << CONFIG_BUDDY_MAX_ORDER)) {
        return BUDDY_ERROR_REQUEST_TOO_LARGE;
    }
    if (block_address < buddy_system.memory_base || block_address >= buddy_system.memory_end) {
        return BUDDY_ERROR_ADDRESS_OUT_OF_RANGE;
    }
    if ((block_address - buddy_system.memory_base) & (BUDDY_PAGE_SIZE - 1)) {
        return BUDDY_ERROR_UNALIGNED_ADDRESS;
    }

    page_index = address_to_page_index(block_address);
    if (page_count > buddy_system.page_count - page_index) {
        return BUDDY_ERROR_ADDRESS_OUT_OF_RANGE;
    }

    /* Validate the complete piece chain before modifying allocator state. */
    current_index = page_index;
    remaining_pages = page_count;
    while (remaining_pages > 0) {
        size_t block_pages;

        if (buddy_system.pages[current_index].state != BUDDY_PAGE_ALLOCATED) {
            return BUDDY_ERROR_NOT_ALLOCATED;
        }

        block_pages = 1UL << buddy_system.pages[current_index].order;
        if (block_pages > remaining_pages) {
            return BUDDY_ERROR_INVALID_ARGUMENT;
        }

        current_index += block_pages;
        remaining_pages -= block_pages;
    }

    LOG_VERBOSE("buddy", "free 0x%08X, %u pages",
                (uint32_t) block_address,
                (uint32_t) page_count);

    current_index = page_index;
    remaining_pages = page_count;
    while (remaining_pages > 0) {
        size_t block_pages = 1UL << buddy_system.pages[current_index].order;
        size_t next_index = current_index + block_pages;

        free_order_block(current_index);
        current_index = next_index;
        remaining_pages -= block_pages;
    }

    return BUDDY_SUCCESS;
}

buddy_state_t buddy_get_state(void)
{
    return buddy_system.state;
}

void buddy_dump_state(void)
{
    size_t free_pages = 0;

    printf("buddy range : [0x%08X, 0x%08X)\n",
           (uint32_t) buddy_system.memory_base,
           (uint32_t) buddy_system.memory_end);
    printf("buddy pages : %u\n", (uint32_t) buddy_system.page_count);
    printf("buddy state : %u\n", (uint32_t) buddy_system.state);

    for (uint32_t order = 0; order < BUDDY_ORDER_COUNT; order++) {
        size_t block_count = buddy_system.free_areas[order].free_count;

        free_pages += block_count << order;
        printf("  order %u: %u free blocks\n", order, (uint32_t) block_count);
    }
    printf("free pages  : %u\n", (uint32_t) free_pages);
}

static uintptr_t page_index_to_address(size_t page_index)
{
    return buddy_system.memory_base + (page_index << CONFIG_BUDDY_PAGE_SHIFT);
}

static size_t address_to_page_index(uintptr_t address)
{
    return (address - buddy_system.memory_base) >> CONFIG_BUDDY_PAGE_SHIFT;
}

static list_head_t *page_index_to_node(size_t page_index)
{
    return (list_head_t *) page_index_to_address(page_index);
}

static size_t node_to_page_index(const list_head_t *node)
{
    return address_to_page_index((uintptr_t) node);
}

static void add_free_block(size_t page_index)
{
    uint32_t order = buddy_system.pages[page_index].order;

    list_add_last(page_index_to_node(page_index), &buddy_system.free_areas[order].free_list);
    buddy_system.free_areas[order].free_count++;
}

static void remove_free_block(size_t page_index)
{
    uint32_t order = buddy_system.pages[page_index].order;

    list_remove(page_index_to_node(page_index));
    buddy_system.free_areas[order].free_count--;
}

static uint32_t page_count_to_order(size_t page_count)
{
    uint32_t order = 0;
    size_t block_pages = 1;

    while (block_pages < page_count) {
        block_pages <<= 1;
        order++;
    }

    return order;
}

static buddy_error_t allocate_order(uint32_t order, size_t *page_index)
{
    uint32_t available_order;
    list_head_t *node;

    for (available_order = order; available_order <= CONFIG_BUDDY_MAX_ORDER; available_order++) {
        if (!list_is_empty(&buddy_system.free_areas[available_order].free_list)) {
            break;
        }
    }
    if (available_order > CONFIG_BUDDY_MAX_ORDER) {
        return BUDDY_ERROR_OUT_OF_MEMORY;
    }

    node = buddy_system.free_areas[available_order].free_list.next;
    *page_index = node_to_page_index(node);
    remove_free_block(*page_index);
    split_free_block(*page_index, order);
    buddy_system.pages[*page_index].state = BUDDY_PAGE_ALLOCATED;
    buddy_system.pages[*page_index].order = order;
    return BUDDY_SUCCESS;
}

static void split_free_block(size_t page_index, uint32_t target_order)
{
    uint32_t order = buddy_system.pages[page_index].order;

    while (order > target_order) {
        size_t buddy_index;

        order--;
        buddy_index = page_index + (1UL << order);
        buddy_system.pages[page_index].state = BUDDY_PAGE_FREE;
        buddy_system.pages[page_index].order = order;
        buddy_system.pages[buddy_index].state = BUDDY_PAGE_FREE;
        buddy_system.pages[buddy_index].order = order;
        add_free_block(buddy_index);
        log_block("split free", buddy_index);
    }
}

static void trim_allocated_block(size_t page_index, size_t page_count)
{
    uint32_t order = buddy_system.pages[page_index].order;
    size_t remaining_pages = page_count;

    while ((1UL << order) > remaining_pages) {
        size_t half_pages;
        size_t buddy_index;

        order--;
        half_pages = 1UL << order;
        buddy_index = page_index + half_pages;
        buddy_system.pages[page_index].state = BUDDY_PAGE_ALLOCATED;
        buddy_system.pages[page_index].order = order;
        buddy_system.pages[buddy_index].state = BUDDY_PAGE_ALLOCATED;
        buddy_system.pages[buddy_index].order = order;

        if (remaining_pages <= half_pages) {
            buddy_system.pages[buddy_index].state = BUDDY_PAGE_FREE;
            add_free_block(buddy_index);
            log_block("release excess", buddy_index);
        } else {
            remaining_pages -= half_pages;
            page_index = buddy_index;
        }
    }
}

static void free_order_block(size_t page_index)
{
    uint32_t order = buddy_system.pages[page_index].order;

    buddy_system.pages[page_index].state = BUDDY_PAGE_FREE;
    log_block("free piece", page_index);

    while (order < CONFIG_BUDDY_MAX_ORDER) {
        size_t buddy_index = page_index ^ (1UL << order);
        size_t upper_index;

        if (buddy_index >= buddy_system.page_count ||
            buddy_system.pages[buddy_index].state != BUDDY_PAGE_FREE ||
            buddy_system.pages[buddy_index].order != order) {
            break;
        }

        remove_free_block(buddy_index);
        upper_index = page_index > buddy_index ? page_index : buddy_index;
        page_index = page_index < buddy_index ? page_index : buddy_index;
        buddy_system.pages[upper_index].state = BUDDY_PAGE_TAIL;
        buddy_system.pages[page_index].state = BUDDY_PAGE_FREE;
        buddy_system.pages[page_index].order = ++order;
        log_block("merge", page_index);
    }

    add_free_block(page_index);
}

static void log_block(const char *operation, size_t page_index)
{
    LOG_VERBOSE("buddy", "%s 0x%08X order %u",
                operation,
                (uint32_t) page_index_to_address(page_index),
                (uint32_t) buddy_system.pages[page_index].order);
}
