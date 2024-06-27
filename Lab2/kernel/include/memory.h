#ifndef MEMORY_H
#define MEMORY_H

#define HEAP_BASE   0x200000
#define HEAP_SIZE   0X100000    // 1 MB heap
#define HEAP_TAIL   (HEAP_BASE + HEAP_SIZE)

unsigned int mem_align(unsigned int target_size, unsigned int align_size);
void* mem_alloc(unsigned int size);

#endif