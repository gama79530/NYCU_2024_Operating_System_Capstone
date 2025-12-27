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

.global initialize
initialize:
    // read cpu id and stop slave cores
    mov     x19, x0                 // x0 is device tree blob address
    bl      get_cpu_id
    cbnz    x0, _hang
    

// from el3 to el2
_el3_init:
    bl      get_el
    cmp     x0, #2
    beq     _el2_init

    // Counter-timer Hypervisor Control register
    mrs     x0, cnthctl_el2
    ldr     x1, =CNTHCTL_EL2
    orr     x0, x0, x1
    msr     cnthctl_el2, x0
 
    // Counter-timer Virtual Offset register
    ldr     x0, =CNTVOFF_EL2
    msr     cntvoff_el2, x0

    // Secure Configuration Register
    ldr     x0, =SCR_EL3
    msr     scr_el3, x0
    
    // Saved Program Status Register (EL3)
    ldr     x0, =SPSR_EL3
    msr     spsr_el3, x0
    
    //  Exception Link Registers (ELR_EL3)
    adr     x0, _el2_init
    msr     elr_el3, x0
    eret

// from el2 to el1
_el2_init:
    bl      get_el
    cmp     x0, #1
    beq     _el1_init

    // Hypervisor Configuration Register
    ldr     x0, =HCR_EL2
    msr     hcr_el2, x0

    // Saved Program Status Register (EL2)
    ldr     x0, =SPSR_EL2           
    msr     spsr_el2, x0

    //  Exception Link Registers (ELR_EL2)
    adr     x0, _el1_init
    msr     elr_el2, x0
    eret

_el1_init:
    // setup vector table
    ldr     x0, =vtable_el1
    msr     vbar_el1, x0

    // System Control Register (EL1)
    ldr     x0, =SCTLR_EL1
    msr     sctlr_el1, x0

    // clear bss segment
    ldr     x0, =bss_begin
    ldr     x1, =bss_size
    bl      memzero

    // initialize stack pointer
    ldr     x9, =kernel_begin
    mov     sp, x9

    // enter kernel
    mov     x0, x19                 // pass device tree blob address
    bl      main
// End of initialize


_hang:
    wfe
    b       _hang
// End of _hang


.global get_el
get_el:
    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    and     x0, x0, #0x3
    ret
// End of get_el


