#include "memory.h"
#include "util.h"

static void *_alloc_cur = (void*)HEAP_BASE;

void* mem_alloc(uint32_t size){
    void *ret_ptr = NULL;
    if(!size){
        return ret_ptr;
    }

    size = mem_align(size, 4);
    if((uint64_t)(_alloc_cur + size) >= HEAP_TAIL){
        return ret_ptr;
    }

    ret_ptr = _alloc_cur;
    _alloc_cur += size;

    return ret_ptr;
}

void mem_free(void *ptr){
    // TODO: finish this function.
}