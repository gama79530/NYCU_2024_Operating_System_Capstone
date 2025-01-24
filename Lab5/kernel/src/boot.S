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
_start:
    // x0 is already used by dtb, so we have to preserve it and then pass it to kernel_main
    mov     x9, x0

    // read cpu id and stop slave cores
    bl      get_cpu_id
    cbnz    x0, _hang

/*  You can find the information on each register field in the chapter 
    "J.3 Functional Index of AArch64 Registers and System Instructions" of the 
    "ARM Architecture Reference Manual ARMv8, for ARMv8-A Architecture Profile."
    
    Rpi3’s CPU runs at EL2 after being booted by default, so the function "from_el3_to_el1" 
    and related invoking code can be omitted.
    
    Read the explanation from the repository
    https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson02/rpi-os.md#sctlr_el1-system-control-register-el1-page-2654-of-aarch64-reference-manual
*/
    bl      get_current_el
    cmp     x0, #3
    beq     from_el3_to_el1

    cmp     x0, #2
    beq     from_el2_to_el1

    b       _hang

_kernel_init:
    // clear bss segment
    ldr     x0, =bss_begin
    ldr     x1, =bss_size
    bl      memzero

    // set top of stack just before our code
    ldr     x0, =_start
    mov     sp, x0
    
    // set exception level 1 vector table 
    adr     x0, vtable_el1
    msr     vbar_el1, x0
    
    // invoke main function
    mov     x0, x9
    bl      kernel_main

_hang:
    wfe
    b       _hang

from_el3_to_el1:
    ldr     x1, =SCTLR_VALUE
    msr     sctlr_el1, x1
    ldr     x1, =HCR_VALUE
    msr     hcr_el2, x1
    ldr     x1, =SCR_VALUE
    msr     scr_el3, x1
    ldr     x1, =SPSR_VALUE
    msr     spsr_el3, x1
    adr     x0, _kernel_init
    msr     elr_el3, x0
    eret  

from_el2_to_el1:
    ldr     x1, =SCTLR_VALUE
    msr     sctlr_el1, x1
    ldr     x1, =HCR_VALUE
    msr     hcr_el2, x1
    ldr     x1, =SPSR_VALUE
    msr     spsr_el2, x1
    adr     x0, _kernel_init
    msr     elr_el2, x0
    eret
