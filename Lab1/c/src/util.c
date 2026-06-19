#include "util.h"

void wait_cycles(uint64_t cycles)
{
    for (uint64_t i = 0; i < cycles; i++) {
        asm volatile("nop");
    }
}
