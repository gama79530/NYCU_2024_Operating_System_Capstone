#include "sched.h"

.global get_current_task
get_current_task:
    mrs     x0, tpidr_el1
    ret

.global set_current_task
set_current_task:
    msr     tpidr_el1, x0
    ret

.global cpu_switch_to
cpu_switch_to:
    mov	    x11, #CPU_CONTEXT_OFFSET
    // store callee-saved registers
    add	    x9, x0, x11
    mov	    x10, sp
    stp	    x19, x20, [x9], #16
    stp	    x21, x22, [x9], #16
    stp	    x23, x24, [x9], #16
    stp	    x25, x26, [x9], #16
    stp	    x27, x28, [x9], #16
    stp	    x29, x30, [x9], #16
    str	    x10, [x9]
    // restore callee-saved registers
    add	    x9, x1, x11
    ldp	    x19, x20, [x9], #16
    ldp	    x21, x22, [x9], #16
    ldp	    x23, x24, [x9], #16
    ldp	    x25, x26, [x9], #16
    ldp	    x27, x28, [x9], #16
    ldp	    x29, x30, [x9], #16
    ldr     x10, [x9]
    mov     sp, x10
	ret
