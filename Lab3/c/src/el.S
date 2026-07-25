#include "arm_v8.h"

.section ".text"

.global el_enter_el0
.type el_enter_el0, %function
el_enter_el0:
    msr     sp_el0, x1

    ldr     x2, =SPSR_EL1_EL0T_DAIF_MASKED
    msr     spsr_el1, x2
    msr     elr_el1, x0
    eret
.size el_enter_el0, . - el_enter_el0
