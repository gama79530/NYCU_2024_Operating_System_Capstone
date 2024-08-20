#include "memory.h"
#include "util.h"

extern char heap_base;
extern char heap_boundary;

void *startup_heap_base = (void*)&heap_base;
void *startup_heap_boundary = (void*)&heap_boundary;
static void *cursor = (void*)&heap_base;

void* startup_memory_alloc(uint64_t size){
    void *ret_ptr = NULL;
    if(!size){
        return ret_ptr;
    }

    size = align_ceiling(size, 4);
    if((uint64_t)cursor + size >= (uint64_t)startup_heap_boundary){
        return ret_ptr;
    }

    ret_ptr = cursor;
    cursor += size;

    return ret_ptr;
}

/* future work */
static void *_alloc_cur = (void*)HEAP_BASE;

void* memory_alloc(uint32_t size){
    void *ret_ptr = NULL;
    if(!size){
        return ret_ptr;
    }

    size = align_ceiling(size, 4);
    if((uint64_t)(_alloc_cur + size) >= HEAP_TAIL){
        return ret_ptr;
    }

    ret_ptr = _alloc_cur;
    _alloc_cur += size;

    return ret_ptr;
}

void memory_free(void *ptr){
    // TODO: finish this function.
}