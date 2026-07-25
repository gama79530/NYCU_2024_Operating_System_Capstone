#ifndef LAB3_C_EL_H
#define LAB3_C_EL_H

#include "types.h"

/* Enter an EL0 program by setting SP_EL0, SPSR_EL1, ELR_EL1, and eret. */
void el_enter_el0(uintptr_t entry, uintptr_t stack);

#endif
