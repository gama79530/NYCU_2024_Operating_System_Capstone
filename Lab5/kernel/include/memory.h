#ifndef __MEMORY_H__
#define __MEMORY_H__
#include "types.h"

void* startup_alloc(uint64_t size);

int memory_sys_init(void);
void* memory_alloc(uint64_t size);
void memory_free(void *ptr);

#define malloc  memory_alloc
#define free    memory_free

#endif