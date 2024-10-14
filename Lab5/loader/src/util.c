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

void wait_msec(uint64_t n){
    uint64_t f, t, r;
    // get the current counter frequency
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(f));
    // read the current counter
    asm volatile ("mrs %0, cntpct_el0" : "=r"(t));
    // calculate required count increase
    unsigned long i = ((f / 1000) * n) / 1000;
    // loop while counter increase is less than i
    do{
        asm volatile ("mrs %0, cntpct_el0" : "=r"(r));
    }while(r - t < i);
}

