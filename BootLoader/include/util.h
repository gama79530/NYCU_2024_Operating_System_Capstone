#ifndef BOOTLOADER_UTIL_H
#define BOOTLOADER_UTIL_H

#include "types.h"

void memzero(void *ptr, size_t size);
uint32_t get32(uintptr_t addr);
void put32(uintptr_t addr, uint32_t value);
void data_memory_barrier(void);
void data_sync_barrier(void);
void instruction_sync_barrier(void);

void wait_cycles(uint64_t cycles);
void sync_instruction_cache(uintptr_t addr, size_t size);

#endif
