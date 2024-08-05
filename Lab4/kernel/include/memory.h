#ifndef MEMORY_H
#define MEMORY_H
#include "types.h"

#define HEAP_BASE   0x200000
#define HEAP_SIZE   0X100000    // 1 MB heap
#define HEAP_TAIL   (HEAP_BASE + HEAP_SIZE)

void* memory_alloc(uint32_t size);
void memory_free(void *ptr);

#define malloc  memory_alloc
#define free    memory_free

#endif