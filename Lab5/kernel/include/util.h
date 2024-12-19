#ifndef __UTIL_H__
#define __UTIL_H__
#include "types.h"
#include <stddef.h>

extern uint64_t get_cpu_id(void);
extern uint64_t get_current_el(void);
extern void memzero(void *ptr, uint64_t size);
extern void memcpy(void *dest, const void *src, size_t n);

#define container_of(ptr, type, member) ((type*)((void*)(ptr) - offsetof(type, member)))

#define swap(a, b) do{\
    __typeof__(a) temp = (a);\
    (a) = (b);\
    (b) = temp;\
}while(0)

uint32_t get32(uint64_t addr);
void put32(uint64_t addr, uint32_t val);
void wait_cycles(uint64_t n);   // Wait N CPU cycles (ARM CPU only)

#define is_power_of_two(n) (n > 0 && (n & (n - 1)) == 0)
uint64_t round_up(uint64_t target, uint64_t unit);
uint64_t truncate(uint64_t target, uint64_t unit);

#define endian_exchange(src_ptr, dst_ptr, type_len) do{\
    uint8_t *src = (uint8_t*)(src_ptr);\
    uint8_t *dst = (uint8_t*)(dst_ptr);\
    for(int i = 0; i < type_len; i++){\
        dst[i] = src[type_len - 1 - i];\
    }\
}while(0)

#endif