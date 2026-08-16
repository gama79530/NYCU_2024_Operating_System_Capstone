#ifndef LAB4_C_ALLOCATOR_H
#define LAB4_C_ALLOCATOR_H

#include "types.h"

/* Initialize the simple bump allocator with the linker-provided heap range. */
void simple_allocator_init(void);

/* Allocate size bytes from the simple heap and return an aligned pointer. */
void *simple_malloc(size_t size);

/* Return the first byte address managed by the simple allocator. */
uintptr_t simple_allocator_begin(void);

/* Return the next unallocated byte address in the simple heap. */
uintptr_t simple_allocator_current(void);

/* Return one-past-the-end address of the simple heap. */
uintptr_t simple_allocator_end(void);

/* Return the number of bytes still available in the simple heap. */
size_t simple_allocator_remaining(void);

#endif
