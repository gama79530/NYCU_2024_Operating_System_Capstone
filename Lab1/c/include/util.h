#ifndef LAB1_C_UTIL_H
#define LAB1_C_UTIL_H

#include "types.h"

/* Implemented in util.S. */
void memzero(void *ptr, size_t size);
uint32_t get32(uintptr_t addr);
void put32(uintptr_t addr, uint32_t value);
void data_memory_barrier(void);

/* Implemented in util.c. */
void wait_cycles(uint64_t cycles);

#endif
