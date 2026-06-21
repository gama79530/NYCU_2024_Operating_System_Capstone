#include "util.h"

/* Private types */

/* Private function declarations */

/* Private data */

/* Function implementations */
void wait_cycles(uint64_t cycles)
{
    for (uint64_t i = 0; i < cycles; i++) {
        asm volatile("nop");
    }
}
