#include "memory.h"

static void *_alloc_cur = (void*)HEAP_BASE;

unsigned int mem_align(unsigned int target_size, unsigned int align_size){
    align_size--;
    return (target_size + align_size) & ~align_size;
}

void* mem_alloc(unsigned int size){
    void *ret_ptr = NULL;
    if(!size){
        return ret_ptr;
    }

    size = mem_align(size, 4);
    if((unsigned long)(_alloc_cur + size) >= HEAP_TAIL){
        return ret_ptr;
    }

    ret_ptr = _alloc_cur;
    _alloc_cur += size;

    return ret_ptr;
}