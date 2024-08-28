#include "memory.h"
#include "util.h"
#include "frame.h"
#include "mini_uart.h"
#include "string.h"
#include "cpio.h"
#include "dtb.h"
#include "frame.h"
#include "list.h"

#define SPIN_TABLE_BASE         0x0000
#define SPIN_TABLE_BOUNDARY     0x1000
#define MEMORY_BASE             0x00000000
#define MEMORY_BOUNDARY         0x3C000000
#define BUDDY_FRAME_ORDER       12
#define BUDDY_ORDER_LIMIT       16
#define CHUNK_MIN_ORDER         3
#define CHUNK_MIN_SIZE          (1 << CHUNK_MIN_ORDER)
#define POOL_NUM                (BUDDY_FRAME_ORDER - CHUNK_MIN_ORDER)

typedef struct chunk_pool_header{
    list_head_t anchor;
    uint8_t     pool_idx;
    uint16_t    free_num;
    uint16_t    allocated_num;
    bool        *is_chunk_allocated;
    void        *chunks_base;
} chunk_pool_header_t;

extern char heap_base;
extern char heap_boundary;
extern char kernel_begin;
extern char kernel_end;

/* for buddy system */
static void *startup_heap_base = (void*)&heap_base;
static void *startup_heap_boundary = (void*)&heap_boundary;
static void *cursor = (void*)&heap_base;
static void *buddy_system_metadata = NULL;
/* for memory system */
static list_head_t *chunk_pools = NULL;

void* startup_memory_alloc(uint64_t size);
void startup_memory_preserve(void *metadata);
 
int memory_system_init(void){
    uart_putln("initialize memory system");
#if VERBOSE == 1
    uart_putln("");
#endif

    buddy_system_metadata = buddy_system_init(
        (void*)MEMORY_BASE,
        (void*)MEMORY_BOUNDARY,
        BUDDY_FRAME_ORDER,
        BUDDY_ORDER_LIMIT,
        startup_memory_alloc,
        startup_memory_preserve
    );
    if(buddy_system_metadata == NULL) return -1;

#if VERBOSE == 1
    buddy_show_layout(buddy_system_metadata);
#endif

    /* initialize chunk pools */
    chunk_pools = (list_head_t*)startup_memory_alloc(POOL_NUM * sizeof(list_head_t));
    if(chunk_pools == NULL){
        uart_putln("memory_system_init: chunk_pools allocation fail.");
        return -1;
    }
    for(int i = 0; i < POOL_NUM; i++){
        chunk_pools[i].prev = chunk_pools[i].next = &chunk_pools[i];
    }

    return 0;
}

void* memory_alloc(uint64_t size){
    void *ret = NULL;
    uint64_t pool_idx;
    for(pool_idx = 0; pool_idx < POOL_NUM - 1 && (CHUNK_MIN_SIZE << pool_idx) < size; pool_idx++);

    /* large size */
    if(pool_idx == POOL_NUM - 1){
#if VERBOSE == true
            uart_putln("memory_alloc: allocate a large space");
#endif
        uint64_t frame_num = (size + sizeof(chunk_pool_header_t) + (1 << BUDDY_FRAME_ORDER) - 1) >> BUDDY_FRAME_ORDER;
        uint64_t buddy_order;
        for(buddy_order = 0; frame_num > (1 << buddy_order); buddy_order++);
        chunk_pool_header_t *frame = (chunk_pool_header_t*)frame_alloc(buddy_system_metadata, buddy_order);
        if(frame == NULL){
#if VERBOSE == true
            uart_putln("memory_alloc: allocate large space fail");
#endif
            return NULL;
        }

        list_add((list_head_t*)frame, chunk_pools[pool_idx].prev, &chunk_pools[pool_idx]);
        frame->pool_idx = pool_idx;
        frame->free_num = 0;
        frame->allocated_num = 1;
        frame->is_chunk_allocated = NULL;
        frame->chunks_base = (void*)frame + sizeof(chunk_pool_header_t);

        ret = frame->chunks_base;
    }else{
        uint64_t chunk_size = CHUNK_MIN_SIZE << pool_idx;

        /* find pool_node */
        chunk_pool_header_t *pool_node;
        for(pool_node = (chunk_pool_header_t*)chunk_pools[pool_idx].next;
            pool_node != (chunk_pool_header_t*)&chunk_pools[pool_idx] && pool_node->free_num == 0;
            pool_node = (chunk_pool_header_t*)((list_head_t*)pool_node)->next
        );

        /* need allocate new pool_node */
        if((list_head_t*)pool_node == &chunk_pools[pool_idx]){
#if VERBOSE == true
            uart_putln("memory_alloc: allocate new pool_node");
#endif
            pool_node = (chunk_pool_header_t*)frame_alloc(buddy_system_metadata, 0);
            if(pool_node == NULL){
#if VERBOSE == true
                uart_putln("memory_alloc: allocate new pool_node fail");
#endif
                return NULL;
            }

            list_add((list_head_t*)pool_node, &chunk_pools[pool_idx], chunk_pools[pool_idx].next);
            pool_node->pool_idx = pool_idx;
            pool_node->free_num = ((1 << BUDDY_FRAME_ORDER) - sizeof(chunk_pool_header_t)) / 
                                  (sizeof(bool) + chunk_size);
            pool_node->allocated_num = 0;
            pool_node->is_chunk_allocated = (bool*)((void*)pool_node + sizeof(chunk_pool_header_t));
            for(uint32_t i = 0; i < pool_node->free_num; i++){
                pool_node->is_chunk_allocated[i] = false;
            }
            pool_node->chunks_base = (void*)pool_node + (1 << BUDDY_FRAME_ORDER) - pool_node->free_num * chunk_size;
        }

        int64_t chunk_idx;
        for(chunk_idx = 0; pool_node->is_chunk_allocated[chunk_idx]; chunk_idx++);
        (pool_node->allocated_num)++;
        (pool_node->free_num)--;
        pool_node->is_chunk_allocated[chunk_idx] = true;
        ret = pool_node->chunks_base + chunk_idx * chunk_size;
        
#if VERBOSE == true
        uart_puts("memory_alloc: allocate from pool");
        uart_puts(uint_to_dec_str(pool_idx));
        uart_puts(" with chunk size = ");
        uart_puts(uint_to_dec_str(chunk_size));
        uart_putln(" bytes");
#endif
    }

    return ret;
}

void memory_free(void *ptr){
    uint64_t frame_size = 1 << BUDDY_FRAME_ORDER;
    chunk_pool_header_t *header = (chunk_pool_header_t*)align_floor((uint64_t)ptr, frame_size);

    /* large size */
    if(header->pool_idx == POOL_NUM - 1){
#if VERBOSE == true
        uart_putln("memory_free: free a large space");
#endif
        frame_free(buddy_system_metadata, (void*)header);
    }else{
        uint64_t chunk_size = CHUNK_MIN_SIZE << header->pool_idx;
#if VERBOSE == true
        uart_puts("memory_free: free a chunk with size = ");
        uart_puts(uint_to_dec_str(chunk_size));
        uart_putln(" bytes");
#endif
        uint64_t chunk_idx = ((uint64_t)ptr - (uint64_t)header->chunks_base) >> (header->pool_idx + CHUNK_MIN_ORDER);
        header->free_num++;
        header->allocated_num--;
        header->is_chunk_allocated[chunk_idx] = false;

        if(header->allocated_num == 0){
#if VERBOSE == true
        uart_putln("memory_free: remove pool_node");
#endif
            list_remove((list_head_t*)header);
            frame_free(buddy_system_metadata, (void*)header);
        }
    }
}

void* startup_memory_alloc(uint64_t size){
    void *ret_ptr = NULL;
    if(!size){
        return ret_ptr;
    }

    size = align_ceiling(size, 8);
    if((uint64_t)cursor + size >= (uint64_t)startup_heap_boundary){
        return ret_ptr;
    }

    ret_ptr = cursor;
    cursor += size;

    return ret_ptr;
}

void startup_memory_preserve(void *metadata){
    buddy_preserve_memory(metadata, (void*)SPIN_TABLE_BASE, (void*)SPIN_TABLE_BOUNDARY, "Preserve spin tables for multicore boot");
    buddy_preserve_memory(metadata, (void*)&kernel_begin, (void*)&kernel_end, "Preserve kernel image space");
    buddy_preserve_memory(metadata, get_cpio_begin_ptr(), get_cpio_end_ptr(), "Preserve CPIO memory");
    buddy_preserve_memory(metadata, get_dtb_ptr(), get_dtb_ptr() + get_dtb_size(), "Preserve Device tree memory");
    buddy_preserve_memory(metadata, startup_heap_base, startup_heap_boundary, "Preserve startup heap");
    buddy_preserve_memory(metadata, (void*)((uint64_t)&kernel_begin - (1 << 10)), (void*)&kernel_begin, "Preserve at least 1 kb for kernel stack");
}

/* will be eliminated */
static void *_alloc_cur = (void*)HEAP_BASE;

void* memory_alloc_temp(uint32_t size){
    void *ret_ptr = NULL;
    if(!size){
        return ret_ptr;
    }

    size = align_ceiling(size, 8);
    if((uint64_t)(_alloc_cur + size) >= HEAP_TAIL){
        return ret_ptr;
    }

    ret_ptr = _alloc_cur;
    _alloc_cur += size;

    return ret_ptr;
}

void memory_free_temp(void *ptr){
    // TODO: finish this function.
}