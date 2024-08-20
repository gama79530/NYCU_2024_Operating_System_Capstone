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
    // read cpu id, stop slave cores
    mrs     x10, mpidr_el1
    and     x10, x10, #0xff
    cbz     x10, _el3_to_el1

_slave_hang:
    wfe
    b       _slave_hang

/*  You can find the information on each register field in the chapter 
    "J.3 Functional Index of AArch64 Registers and System Instructions" of the 
    "ARM Architecture Reference Manual ARMv8, for ARMv8-A Architecture Profile."
    
    Rpi3’s CPU runs at EL2 after being booted by default, so the following "_el3_to_el1" setting can be omitted. 
    Read the explanation from the repository
    https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson02/rpi-os.md#sctlr_el1-system-control-register-el1-page-2654-of-aarch64-reference-manual
*/ 

_el3_to_el1:
    mrs     x10, CurrentEL
    lsr     x10, x10, #2
    cmp     x10, #3
    bne     _el2_to_el1

    bl      from_el3_to_el1

_el2_to_el1:
    mrs     x10, CurrentEL
    lsr     x10, x10, #2
    cmp     x10, #2
    bne     _kernel_init

    bl      from_el2_to_el1

_kernel_init:
    // clear bss segment
    ldr     x10, =bss_begin
    ldr     x11, =bss_size
    bl      bss_init

    // set top of stack just before our code
    ldr     x11, =_start
    mov     sp, x11

    // set exception level 1 vector table 
    adr     x10, vtable_el1
    msr     vbar_el1, x10

    // invoke main function
    bl      main

_master_hang:
    wfe
    b       _master_hang

from_el3_to_el1:
    ldr     x10, =0x30D00800     // SCTLR_VALUE_MMU_DISABLED
    msr     sctlr_el1, x10
    ldr     x10, =0x80000000     // HCR_VALUE
    msr     hcr_el2, x10
    ldr     x10, =0x00000431     // SCR_VALUE
    msr     scr_el3, x10
    ldr     x10, =0x000001C5     // SPSR_VALUE
    msr     spsr_el3, x10
    adr     x10, _kernel_init
    msr     elr_el3, x10
    eret  

from_el2_to_el1:
    mov     x10, (1 << 31)      // EL1 uses aarch64
    msr     hcr_el2, x10
    mov     x10, 0x3c5          // 0011 1100 0101 => EL1h (SPSel = 1) with interrupt disabled
    msr     spsr_el2, x10
    adr     x10, _kernel_init
    msr     elr_el2, x10
    eret

bss_init:
    cbz     x11, 1f
    str     xzr, [x10], #8
    sub     x11, x11, #1
    b       bss_init
1:
    ret

