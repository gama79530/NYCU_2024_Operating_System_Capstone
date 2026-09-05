#ifndef LAB4_C_ALLOCATOR_H
#define LAB4_C_ALLOCATOR_H

#include "types.h"

/* Minimum alignment guaranteed by every kernel allocator. */
#define KERNEL_ALLOC_ALIGNMENT 8UL

_Static_assert((KERNEL_ALLOC_ALIGNMENT & (KERNEL_ALLOC_ALIGNMENT - 1)) == 0,
               "kernel allocator alignment must be a power of two");

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

/* Initialize the dynamic allocator after the buddy system is ready. */
bool kernel_allocator_init(void);

/* Return whether allocations are currently served by the dynamic allocator. */
bool kernel_allocator_is_ready(void);

/* Allocate memory from the active startup or dynamic allocator. */
void *kernel_malloc(size_t size);

/* Release a dynamic allocation; startup allocations remain permanent. */
void kernel_free(void *address);

#define malloc kernel_malloc
#define free   kernel_free

#endif
