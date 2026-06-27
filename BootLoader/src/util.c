#include "util.h"

#define CACHE_WORD_SIZE 4UL

void wait_cycles(uint64_t cycles)
{
    for (uint64_t i = 0; i < cycles; i++) {
        asm volatile("nop");
    }
}

void sync_instruction_cache(uintptr_t addr, size_t size)
{
    uintptr_t end;
    uint64_t ctr;
    size_t dline_size;
    size_t iline_size;

    if (size == 0) {
        return;
    }

    end = addr + size;
    asm volatile("mrs %0, ctr_el0" : "=r"(ctr));
    dline_size = CACHE_WORD_SIZE << ((ctr >> 16) & 0xf);
    iline_size = CACHE_WORD_SIZE << (ctr & 0xf);

    for (uintptr_t line = addr & ~(uintptr_t)(dline_size - 1); line < end; line += dline_size) {
        asm volatile("dc cvau, %0" : : "r"(line) : "memory");
    }
    asm volatile("dsb ish" ::: "memory");

    for (uintptr_t line = addr & ~(uintptr_t)(iline_size - 1); line < end; line += iline_size) {
        asm volatile("ic ivau, %0" : : "r"(line) : "memory");
    }
    asm volatile("dsb ish" ::: "memory");
    asm volatile("isb" ::: "memory");
}
