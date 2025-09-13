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
    mov     x19, x0                 // x0 is device tree blob address
    bl      get_cpu_id              // read cpu id and pause slave cores
    cbnz    x0, wfe_before_relocate

    // relocate the loader
    ldr     x0, =loader_begin       // relocate destination address
    ldr     x1, =kernel_begin       // relocate source address
    ldr     x2, =loader_size        // relocate size
    bl      relocate
    SEV

    // clear bss segment
    ldr     x0, =bss_begin
    ldr     x1, =bss_size
    bl      memzero

    // initialize stack pointer
    ldr     x9, =loader_begin
    mov     sp, x9

loader_main:
    mov     x0, x19                 // pass device tree blob address
    adr     x9, main                // caculate relocate main address
    ldr     x10, =relocate_offset
    sub     x11, x9, x10
    blr     x11                     // jump to relocate main

relocate:
1:  // copy 8 bytes a time
    ldr     x9, [x1], #8
    str     x9, [x0], #8
    sub     x2, x2, #8
    cbnz    x2, 1b
    ret
    
wfe_before_relocate:
    WFE
    adr     x9, wfe_after_relocate
    blr     x9

wfe_after_relocate:
    WFE
    ldr     x9, =kernel_begin
    blr     x9
