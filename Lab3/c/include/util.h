#ifndef LAB3_C_UTIL_H
#define LAB3_C_UTIL_H

#include "types.h"

static inline uintptr_t align_up(uintptr_t value, uintptr_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

static inline uint32_t read_be32(const void *ptr)
{
    const uint8_t *bytes = ptr;

    return ((uint32_t) bytes[0] << 24) |
           ((uint32_t) bytes[1] << 16) |
           ((uint32_t) bytes[2] << 8) |
           (uint32_t) bytes[3];
}

static inline uint64_t read_be64(const void *ptr)
{
    const uint8_t *bytes = ptr;

    return ((uint64_t) read_be32(bytes) << 32) | (uint64_t) read_be32(bytes + 4);
}

/* Implemented in util.S. */
void memzero(void *ptr, size_t size);
uint32_t get32(uintptr_t addr);
void put32(uintptr_t addr, uint32_t value);
void data_memory_barrier(void);
void data_sync_barrier(void);

/* Implemented in util.c. */
void wait_cycles(uint64_t cycles);

#endif
