#include "arm_v8.h"

.section ".text"

/*
 * Enter an EL0 program.
 *
 * x0: EL0 entry address.
 * x1: EL0 stack pointer.
 */
.global el_enter_el0
.type el_enter_el0, %function
el_enter_el0:
    msr     sp_el0, x1

    ldr     x2, =SPSR_EL1_EL0T_DAIF_MASKED
    msr     spsr_el1, x2
    msr     elr_el1, x0
    eret
.size el_enter_el0, . - el_enter_el0

/* Default EL0 return target: returning from a user entry invokes exit(0). */
.global el0_thread_return
.type el0_thread_return, %function
el0_thread_return:
    mov x0, #0
    mov x8, #5
    svc #0
    b el0_thread_return
.size el0_thread_return, . - el0_thread_return

/* A user signal handler returns here; syscall 10 restores its interrupted frame. */
.global el0_signal_return
.type el0_signal_return, %function
el0_signal_return:
    mov x8, #10
    svc #0
    b el0_signal_return
.size el0_signal_return, . - el0_signal_return

/* Linker metadata: this object does not request an executable stack. */
.section .note.GNU-stack,"",%progbits
