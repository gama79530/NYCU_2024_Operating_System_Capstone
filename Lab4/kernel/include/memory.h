#ifndef MEMORY_H
#define MEMORY_H
#include "types.h"

#define VERBOSE     true

int memory_system_init(void);
void* memory_alloc(uint64_t size);
void memory_free(void *ptr);

#define malloc  memory_alloc
#define free    memory_free

#endif