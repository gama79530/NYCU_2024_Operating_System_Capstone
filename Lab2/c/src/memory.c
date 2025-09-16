#include "memory.h"
#include "common.h"

extern char startup_heap_base;
extern char startup_heap_boundary;

static char *startup_heap_end = &startup_heap_base;

void *startup_alloc(uint64_t size)
{
    size = round_up(size, 8);  // 8-byte align
    if (startup_heap_end + size > &startup_heap_boundary) {
        return NULL;
    }
    
    void *ret = startup_heap_end;
    startup_heap_end += size;
    return ret;
}