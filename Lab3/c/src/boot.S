#include "arm_v8.h"

/*
    Procedure Call Standard
    ref: https://developer.arm.com/documentation/102374/0102/Procedure-Call-Standard
    - Parameter and Result Registers (X0-X7)
    - Corruptible Registers (X9-X15)
    - Callee-saved Registers (X19-X28)
    - Registers with a special purpose (X8, X16-X18, X29 [FP], X30 [LR])
*/
.section ".text.boot"

.global _start
.type _start, %function
_start:
    // Keep the firmware or bootloader-provided DTB pointer for C code.
    mov     x19, x0

    // Only core 0 enters the kernel.
    mrs     x0, mpidr_el1
    and     x0, x0, #0xff
    cbnz    x0, hang

    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    cmp     x0, #1
    beq     el1_entry
    cmp     x0, #2
    beq     setup_el2_to_el1
    b       hang

el1_entry:
    // The stack grows downward from the kernel load address.
    ldr     x0, =_start
    mov     sp, x0

    // Clear the BSS segment.
    ldr     x0, =bss_begin
    ldr     x1, =bss_size
    bl      memzero

    // Enter the C kernel.
    mov     x0, x19
    bl      main

setup_el2_to_el1:
    // Run the lower exception level in AArch64.
    ldr     x0, =HCR_EL2_VALUE
    msr     hcr_el2, x0

    // Return to EL1h with DAIF masked.
    ldr     x0, =SPSR_EL2_VALUE
    msr     spsr_el2, x0

    adr     x0, el1_entry
    msr     elr_el2, x0
    eret

hang:
    wfe
    b       hang

.size _start, . - _start
