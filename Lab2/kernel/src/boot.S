.section ".text.boot"

.global _start

_start:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #0xff
    cbnz    x1, _hang

    // clear bss segment
    ldr     x1, =bss_begin
    ldr     x2, =bss_size

_bss_init:
    cbz     x2, _init_sp
    str     xzr, [x1], #8
    sub     x2, x2, #1
    b       _bss_init

_init_sp:
    // set top of stack just before our code
    ldr     x1, =_start
    mov     sp, x1

_main:
    bl      main

_hang:
    wfe
    b       _hang
