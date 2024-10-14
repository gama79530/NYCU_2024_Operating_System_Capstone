/*
    Parameters in general-purpose registers 
    ref: https://blog.csdn.net/u014100559/article/details/113105083
    
    - Argument registers (X0-X7)
    - Caller-saved temporary registers (X9-X15)
    - Callee-saved registers (X19-X29)
    - Registers with a special purpose (X8, X16-X18, X29, X30)
*/
.section ".text.boot"

.global _start
_start:
    // x0 is already used by dtb, so we have to save it so that we can pass it to kernel
    mov     x19, x0

    // read cpu id and stop slave cores
    bl      get_cpu_id
    cbnz    x0, _hang

/*  You can find the information on each register field in the chapter 
    "J.3 Functional Index of AArch64 Registers and System Instructions" of the 
    "ARM Architecture Reference Manual ARMv8, for ARMv8-A Architecture Profile."
    
    Rpi3’s CPU runs at EL2 after being booted by default, so the following "_el3" setting can be omitted. 
    Read the explanation from the repository
    https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson02/rpi-os.md#sctlr_el1-system-control-register-el1-page-2654-of-aarch64-reference-manual
*/
    bl      get_current_el
    cmp     x0, #3
    beq     _el3
    cmp     x0, #2
    beq     _el2

_el3:
    bl      from_el3_to_el1

_el2:
    bl      from_el2_to_el1

_kernel_init:
    // clear bss segment
    ldr     x0, =bss_begin
    ldr     x1, =bss_size
    bl      bss_init

    // set top of stack just before our code
    ldr     x0, =_start
    mov     sp, x0
    
    // set exception level 1 vector table 
    adr     x0, vtable_el1
    msr     vbar_el1, x0
    
    // invoke main function
    mov     x0, x19
    bl      kernel_main

_hang:
    wfe
    b       _hang

from_el3_to_el1:
    ldr     x0, =0x30D00800     // SCTLR_VALUE_MMU_DISABLED
    msr     sctlr_el1, x0
    ldr     x0, =0x80000000     // HCR_VALUE
    msr     hcr_el2, x0
    ldr     x0, =0x00000431     // SCR_VALUE
    msr     scr_el3, x0
    ldr     x0, =0x000001C5     // SPSR_VALUE
    msr     spsr_el3, x0
    adr     x0, _kernel_init
    msr     elr_el3, x0
    eret  

from_el2_to_el1:
    mov     x0, (1 << 31)      // EL1 uses aarch64
    msr     hcr_el2, x0
    mov     x0, 0x3c5          // 0011 1100 0101 => EL1h (SPSel = 1) with interrupt disabled
    msr     spsr_el2, x0
    adr     x0, _kernel_init
    msr     elr_el2, x0
    eret

bss_init:
    cbz     x1, 1f
    str     xzr, [x0], #8
    sub     x1, x1, #1
    b       bss_init
1:
    ret