#include "util.h"
#include "common.h"

void wait_cycles(uint64_t n)
{
    for (uint64_t i = 0; i < n; i++) {
        asm volatile("nop");
    }
}

void wait_msec(uint64_t msec)
{
    register uint64_t frequency, start, now;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(frequency));  // get the counter frequency
    asm volatile("mrs %0, cntpct_el0" : "=r"(start));      // read the current count
    uint64_t require_count =
        ((frequency / 1000) * msec) / 1000;  // calculate required count increase
    do {
        asm volatile("mrs %0, cntpct_el0" : "=r"(now));
    } while (now - start < require_count);
}
