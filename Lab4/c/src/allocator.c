#include "allocator.h"

#include "config.h"
#include "util.h"

/* Private types */

/* Private function declarations */

/* Private data */

extern char simple_heap_begin;
extern char simple_heap_end;

static uintptr_t heap_begin;
static uintptr_t heap_current;
static uintptr_t heap_end;

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
    next = align_up(allocated + size, CONFIG_SIMPLE_ALLOCATOR_ALIGNMENT);

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
