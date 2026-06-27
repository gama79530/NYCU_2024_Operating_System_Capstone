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
    // Only core 0 enters the kernel.
    mrs     x0, mpidr_el1
    and     x0, x0, #0xff
    cbnz    x0, hang

    // The stack grows downward from the kernel load address.
    ldr     x0, =_start
    mov     sp, x0

    // Clear the BSS segment.
    ldr     x0, =bss_begin
    ldr     x1, =bss_size
    bl      memzero

    // Enter the C kernel.
    bl      main

hang:
    wfe
    b       hang

.size _start, . - _start
