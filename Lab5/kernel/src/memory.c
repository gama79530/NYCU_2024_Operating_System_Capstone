#include "memory.h"
#include "util.h"
#include "frame.h"
#include "printf.h"
#include "config.h"
#include "list.h"

typedef struct chunk_pool_header{
    list_head_t head;
    void        *chunks_base;
    bool        *chunk_is_allocated;
    uint16_t    free_num;
    uint16_t    allocated_num;
    uint8_t     pool_idx;
} chunk_pool_header_t;

extern char startup_heap_base;
extern char startup_heap_boundary;
static void *startup_heap_end = (void*)&startup_heap_base;

static list_head_t *chunk_pools = NULL;

void* startup_alloc(uint64_t size){
    size = round_up(size, 8);
    if((uint64_t)startup_heap_end + size > (uint64_t)&startup_heap_boundary){
#if VERBOSE != 0
        printf("startup_alloc: startup_heap is used up.\n");
#endif
        return NULL;
    }
    void *ret = startup_heap_end;
    startup_heap_end += size;
    return ret;
}

int memory_sys_init(void){
    if(get_buddy_sys_state() != 2){
#if VERBOSE != 0
        printf("memory_sys_init: buddy system is not built yet.\n");
#endif
        return -1;
    }else if(chunk_pools != NULL){
#if VERBOSE != 0
        printf("memory_sys_init: can not initialize memory system twice.\n");
#endif
    }

    chunk_pools = (list_head_t*)startup_alloc(POOL_NUM * sizeof(list_head_t));
    if(chunk_pools == NULL){
#if VERBOSE != 0
        printf("memory_sys_init: chunk_pools allocation fail.\n");
#endif
        return -1;
    }
    for(int i = 0; i < POOL_NUM; i++){
        chunk_pools[i].prev = chunk_pools[i].next = chunk_pools + i;
    }

    return 0;
}

void* memory_alloc(uint64_t size){
    if(chunk_pools == NULL){
#if VERBOSE != 0
        printf("memory_alloc: memory system is not initialized.\n");
#endif
        return NULL;
    }

    void *ret = NULL;
    uint8_t pool_idx;

    for(pool_idx = 0; pool_idx < POOL_NUM - 1 && (CHUNK_MIN_SIZE << pool_idx) < size; pool_idx++);

    /* large size */
    if(pool_idx == (uint8_t)POOL_NUM - 1){
#if VERBOSE != 0
        printf("memory_alloc: allocate a large space.\n");
#endif
        uint64_t frame_num = (size + sizeof(chunk_pool_header_t) + FRAME_SIZE - 1) >> FRAME_ORDER;
        uint8_t buddy_order = 0;
        for(buddy_order = 0; frame_num > (1L << buddy_order); buddy_order++);
        chunk_pool_header_t *buddy_group = (chunk_pool_header_t*)frame_alloc(buddy_order);
        if(buddy_group == NULL) return NULL;
        list_append(&buddy_group->head, chunk_pools + pool_idx);
        buddy_group->pool_idx = pool_idx;
        buddy_group->free_num = 0;
        buddy_group->allocated_num = 1;
        buddy_group->chunk_is_allocated = NULL;
        ret = buddy_group->chunks_base = (void*)buddy_group + sizeof(chunk_pool_header_t);
    }else{
        uint64_t chunk_size = CHUNK_MIN_SIZE << pool_idx;

        /* find pool_node */
        list_head_t *node;
        for(node = chunk_pools[pool_idx].next;
            node != chunk_pools + pool_idx && container_of(node, chunk_pool_header_t, head)->free_num == 0;
            node = node->next
        );

        chunk_pool_header_t *pool_node;
        /* need allocate new pool_node */
        if(node == chunk_pools + pool_idx){
#if VERBOSE != 0
            printf("memory_alloc: allocate a new pool_node.\n");
#endif
            pool_node = (chunk_pool_header_t*)frame_alloc(0);
            if(pool_node == NULL)   return NULL;

            list_append(&pool_node->head, chunk_pools + pool_idx);
            pool_node->pool_idx = pool_idx;
            pool_node->free_num = (FRAME_SIZE - sizeof(chunk_pool_header_t)) / (sizeof(bool) + chunk_size);
            pool_node->allocated_num = 0;
            pool_node->chunk_is_allocated = (bool*)((void*)pool_node + sizeof(chunk_pool_header_t));
            for(uint16_t i = 0; i < pool_node->free_num; i++)
                pool_node->chunk_is_allocated[i] = false;
            pool_node->chunks_base = (void*)pool_node + FRAME_SIZE - pool_node->free_num * chunk_size;
        }else{
#if VERBOSE != 0
            printf("memory_alloc: find chunk from an existing pool_node.\n");
#endif
            pool_node = container_of(node, chunk_pool_header_t, head);
        }
        
        uint16_t chunk_idx;
        for(chunk_idx = 0; pool_node->chunk_is_allocated[chunk_idx]; chunk_idx++);
        (pool_node->allocated_num)++;
        (pool_node->free_num)--;
        pool_node->chunk_is_allocated[chunk_idx] = true;
        ret = pool_node->chunks_base + (chunk_idx << (CHUNK_MIN_ORDER + pool_idx));

#if VERBOSE != 0
        printf("memory_alloc: allocate from pool %d with chunk size = %d bytes.\n", pool_idx, chunk_size);
#endif
    }

    return ret;
}

void memory_free(void *ptr){
    if(chunk_pools == NULL){
#if VERBOSE != 0
        printf("memory_alloc: memory system is not initialized.\n");
#endif
        return;
    }

    chunk_pool_header_t *header = (chunk_pool_header_t*)truncate((uint64_t)ptr, FRAME_SIZE);
    
    /* large size */
    if(header->pool_idx == POOL_NUM - 1){
#if VERBOSE != 0
        printf("memory_free: free a large space.\n");
#endif
        frame_free((void*)header);
    }else{
#if VERBOSE != 0
        printf("memory_free: free a chunk with size = %d bytes.\n", 1 << (CHUNK_MIN_ORDER + header->pool_idx));
#endif
        uint64_t chunk_idx = ((uint64_t)ptr - (uint64_t)header->chunks_base) >> (header->pool_idx + CHUNK_MIN_ORDER);
        header->free_num++;
        header->allocated_num--;
        header->chunk_is_allocated[chunk_idx] = false;

        if(header->allocated_num == 0){
#if VERBOSE != 0
        printf("memory_free: remove a pool_node\n");
#endif
            list_remove(&header->head);
            frame_free((void*)header);
        }
    }
}