#ifndef KERNEL_LOADER_UTIL_H
#define KERNEL_LOADER_UTIL_H
#include "types.h"

extern uint64_t get_cpu_id(void);
extern void memzero(void *ptr, uint64_t size);
extern uint32_t get32(uint64_t addr);
extern void put32(uint64_t addr, uint32_t val);

void wait_cycles(uint64_t n);
void wait_msec(uint64_t msec);

#define swap(a, b)                \
    do {                          \
        __typeof__(a) temp = (a); \
        (a) = (b);                \
        (b) = temp;               \
    } while (0)

#endif
