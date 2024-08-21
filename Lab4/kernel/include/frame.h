#ifndef FRAME_H
#define FRAME_H
#include "types.h"

#define PRINT_FRAME_MSG     1
#define MEMORY_BASE         0x00000000
#define MEMORY_BOUNDARY     0x3C000000

/* 4kb frame size */
#define FRAME_SIZE_ORDER    12  // requirement: a positive number
#define FRAME_SIZE          (1 << FRAME_SIZE_ORDER)

int buddy_system_init();

void* frame_alloc(uint64_t size);
void frame_free(void *ptr);

#endif