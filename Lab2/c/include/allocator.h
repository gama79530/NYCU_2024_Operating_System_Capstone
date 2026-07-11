#ifndef LAB2_C_ALLOCATOR_H
#define LAB2_C_ALLOCATOR_H

#include "types.h"

void simple_allocator_init(void);
void *simple_malloc(size_t size);
uintptr_t simple_allocator_begin(void);
uintptr_t simple_allocator_current(void);
uintptr_t simple_allocator_end(void);
size_t simple_allocator_remaining(void);

#endif
