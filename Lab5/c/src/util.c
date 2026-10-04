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

void wait_cycles(uint64_t cycles)
{
    for (uint64_t i = 0; i < cycles; i++) {
        asm volatile("nop");
    }
}
