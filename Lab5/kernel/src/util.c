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

uint64_t round_up(uint64_t target, uint64_t unit){
    return is_power_of_two(unit) ? (target + unit - 1) & ~(unit - 1) : ((target + unit - 1) / unit) * unit;
}

uint64_t truncate(uint64_t target, uint64_t unit){
    return is_power_of_two(unit) ? target & ~(unit - 1) : (target / unit) * unit;
}
