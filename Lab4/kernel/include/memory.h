#ifndef MEMORY_H
#define MEMORY_H
#include "types.h"

#define STARTUP_HEAP_SIZE   0X100000    // 1 MB startup heap

void* startup_memory_alloc(uint64_t size);

#define HEAP_BASE           0x2000000
#define HEAP_SIZE           0X100000    // 1 MB heap
#define HEAP_TAIL           (HEAP_BASE + HEAP_SIZE)

void* memory_alloc(uint32_t size);
void memory_free(void *ptr);

#define malloc  memory_alloc
#define free    memory_free

#endif