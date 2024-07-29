#include "util.h"

uint32_t get32(uint64_t addr){
    uint32_t val = *((volatile uint32_t*)addr);
    return val;
}

void put32(uint64_t addr, uint32_t val){
    *((volatile uint32_t*)addr) = val;
}

void wait_cycles(uint64_t n){
    for(uint64_t i = 0; i < n; i++){
        asm volatile("nop");
    }
}

uint64_t mem_align(uint64_t target, uint64_t align){
    return (target + align - 1) & ~(align - 1);
}

#define _to_little(u_ptr, ret_ptr, bytes){\
    for(int i = 0; i < bytes; i++){\
        (ret_ptr)[i] = (u_ptr)[(bytes) - i - 1];\
    }\
}

uint32_t to_little_u32(const uint32_t u32){
    uint32_t ret = 0;
    _to_little((uint8_t*)&u32, (uint8_t*)&ret, sizeof(uint32_t));
    return ret;
}

uint64_t to_little_u64(const uint64_t u64){
    uint64_t ret = 0;
    _to_little((uint8_t*)&u64, (uint8_t*)&ret, sizeof(uint64_t))
    return ret;
}
