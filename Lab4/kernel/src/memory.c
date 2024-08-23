#include "memory.h"
#include "util.h"
#include "frame.h"
#include "mini_uart.h"
#include "string.h"
#include "cpio.h"
#include "dtb.h"

#define SPIN_TABLE_BASE         0x0000
#define SPIN_TABLE_BOUNDARY     0x1000

extern char heap_base;
extern char heap_boundary;
extern char kernel_begin;
extern char kernel_end;

static void *startup_heap_base = (void*)&heap_base;
static void *startup_heap_boundary = (void*)&heap_boundary;
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

void startup_memory_preserve(void *metadata){
    buddy_preserve_memory(metadata, (void*)SPIN_TABLE_BASE, (void*)SPIN_TABLE_BOUNDARY, "Preserve spin tables for multicore boot");
    buddy_preserve_memory(metadata, (void*)&kernel_begin, (void*)&kernel_end, "Preserve kernel image space");
    buddy_preserve_memory(metadata, get_cpio_begin_ptr(), get_cpio_end_ptr(), "Preserve CPIO memory");
    buddy_preserve_memory(metadata, get_dtb_ptr(), get_dtb_ptr() + get_dtb_size(), "Preserve Device tree memory");
    buddy_preserve_memory(metadata, (void*)&startup_heap_base, (void*)&startup_heap_boundary, "Preserve startup heap");
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