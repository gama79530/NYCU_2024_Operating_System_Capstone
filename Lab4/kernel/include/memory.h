#ifndef MEMORY_H
#define MEMORY_H
#include "types.h"

#define VERBOSE                 true

int memory_system_init(void);
void* memory_alloc(uint64_t size);
void memory_free(void *ptr);

#define malloc  memory_alloc
#define free    memory_free
// #define malloc  memory_alloc_temp
// #define free    memory_free_temp

/* will be eliminated */
#define STARTUP_HEAP_SIZE   0x100000    // 1 MB startup heap

#define HEAP_BASE           0x2000000
#define HEAP_SIZE           0x100000    // 1 MB heap
#define HEAP_TAIL           (HEAP_BASE + HEAP_SIZE)

void* memory_alloc_temp(uint32_t size);
void memory_free_temp(void *ptr);


#endif