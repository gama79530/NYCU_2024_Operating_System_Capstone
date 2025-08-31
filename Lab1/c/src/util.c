#include "util.h"
#include "common.h"

void wait_cycles(uint64_t n)
{
    for (uint64_t i = 0; i < n; i++) {
        asm volatile("nop");
    }
}
