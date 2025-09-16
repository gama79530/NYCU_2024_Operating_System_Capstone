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
    cbnz    x0, hang

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

hang:
    wfe
    b       hang
