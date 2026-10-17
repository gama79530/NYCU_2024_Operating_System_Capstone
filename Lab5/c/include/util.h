#ifndef LAB5_C_UTIL_H
#define LAB5_C_UTIL_H

#include "types.h"

#define container_of(ptr, type, member) ((type *) ((void *) (ptr) - offsetof(type, member)))

/** Round value upward to a multiple of alignment; alignment must be a nonzero power of two. */
static inline uintptr_t align_up(uintptr_t value, uintptr_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

/** Round value downward to a multiple of alignment; alignment must be a nonzero power of two. */
static inline uintptr_t align_down(uintptr_t value, uintptr_t alignment)
{
    return value & ~(alignment - 1);
}

/* Read an unaligned big-endian 32-bit integer from bytes. */
static inline uint32_t read_be32(const void *ptr)
{
    const uint8_t *bytes = ptr;

    return ((uint32_t) bytes[0] << 24) |
           ((uint32_t) bytes[1] << 16) |
           ((uint32_t) bytes[2] << 8) |
           (uint32_t) bytes[3];
}

/* Read an unaligned big-endian 64-bit integer from bytes. */
static inline uint64_t read_be64(const void *ptr)
{
    const uint8_t *bytes = ptr;

    return ((uint64_t) read_be32(bytes) << 32) | (uint64_t) read_be32(bytes + 4);
}

/* Implemented in util.S. */
/* Zero size bytes starting at ptr. */
void memzero(void *ptr, size_t size);

/* Read a 32-bit memory-mapped I/O register. */
uint32_t get32(uintptr_t addr);

/* Write a 32-bit memory-mapped I/O register. */
void put32(uintptr_t addr, uint32_t value);

/* Issue a data memory barrier. */
void data_memory_barrier(void);

/* Issue a data synchronization barrier. */
void data_sync_barrier(void);

/* Implemented in util.c. */
/** Copy size bytes between non-overlapping memory ranges; return dest.
 * Byte-wise access supports unaligned addresses. Not for memory-mapped I/O. */
void *memcpy(void *dest, const void *src, size_t size);

/** Synchronize copied instructions with the I-cache before executing them in EL1/EL0. */
void instruction_cache_sync(uintptr_t begin, size_t size);

/* Busy-wait for approximately cycles loop iterations. */
void wait_cycles(uint64_t cycles);

#endif
