#include "util.h"

/* Private types */

/* Private function declarations */

/* Private data */

/* Function implementations */

void *memcpy(void *dest, const void *src, size_t size)
{
    uint8_t *destination = dest;
    const uint8_t *source = src;

    for (size_t i = 0; i < size; i++) {
        destination[i] = source[i];
    }
    return dest;
}

void instruction_cache_sync(uintptr_t begin, size_t size)
{
    uint64_t ctr;
    uintptr_t end = begin + size;
    size_t data_line;
    size_t instruction_line;

    asm volatile("mrs %0, ctr_el0" : "=r"(ctr));
    /* CTR_EL0 reports cache line sizes as log2(number of 4-byte words). */
    data_line = 4UL << ((ctr >> 16) & 0xf);
    instruction_line = 4UL << (ctr & 0xf);
    for (uintptr_t address = align_down(begin, data_line); address < end; address += data_line) {
        asm volatile("dc cvau, %0" :: "r"(address) : "memory");
    }
    asm volatile("dsb ish" ::: "memory");
    for (uintptr_t address = align_down(begin, instruction_line); address < end;
         address += instruction_line) {
        asm volatile("ic ivau, %0" :: "r"(address) : "memory");
    }
    asm volatile("dsb ish\nisb" ::: "memory");
}

void wait_cycles(uint64_t cycles)
{
    for (uint64_t i = 0; i < cycles; i++) {
        asm volatile("nop");
    }
}
