#include "types.h"

#define swap(a, b) do{\
    __typeof__(a) temp = (a);\
    (a) = (b);\
    (b) = temp;\
}while(0)

uint32_t get32(uint64_t addr);
void put32(uint64_t addr, uint32_t val);

void wait_cycles(uint64_t n);   // Wait N CPU cycles (ARM CPU only)
void wait_msec(uint64_t n);     // Wait N microsec (ARM CPU only)

uint64_t align_ceiling(uint64_t target, uint64_t align);
uint64_t align_floor(uint64_t target, uint64_t align);

uint32_t to_little_u32(const uint32_t u32);
uint64_t to_little_u64(const uint64_t u64);
