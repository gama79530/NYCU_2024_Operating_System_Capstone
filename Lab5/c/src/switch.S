#include "thread.h"

.section ".text"

/* x0: previous thread, x1: next thread. IRQ remains masked across the switch. */
.global switch_to
.type switch_to, %function
switch_to:
    stp x19, x20, [x0, #THREAD_CONTEXT_X19_X20]
    stp x21, x22, [x0, #THREAD_CONTEXT_X21_X22]
    stp x23, x24, [x0, #THREAD_CONTEXT_X23_X24]
    stp x25, x26, [x0, #THREAD_CONTEXT_X25_X26]
    stp x27, x28, [x0, #THREAD_CONTEXT_X27_X28]
    stp x29, x30, [x0, #THREAD_CONTEXT_FP_LR]
    mov x9, sp
    str x9, [x0, #THREAD_CONTEXT_SP]

    ldp x19, x20, [x1, #THREAD_CONTEXT_X19_X20]
    ldp x21, x22, [x1, #THREAD_CONTEXT_X21_X22]
    ldp x23, x24, [x1, #THREAD_CONTEXT_X23_X24]
    ldp x25, x26, [x1, #THREAD_CONTEXT_X25_X26]
    ldp x27, x28, [x1, #THREAD_CONTEXT_X27_X28]
    ldp x29, x30, [x1, #THREAD_CONTEXT_FP_LR]
    ldr x9, [x1, #THREAD_CONTEXT_SP]
    mov sp, x9
    msr tpidr_el1, x1
    ret
.size switch_to, . - switch_to

/*
 * GNU linker metadata: this object does not require an executable stack.
 * The empty flags omit the executable flag; this section allocates no stack.
 * Our linker script discards .note.* sections. This declaration does not
 * enforce stack execution permissions in the bare-metal kernel.
 */
.section .note.GNU-stack,"",%progbits
