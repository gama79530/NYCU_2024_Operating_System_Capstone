.section ".text.boot"

.global _start

_start:
    adr     x10, .                  // load address (default  : 0x80000) 
    ldr     x11, =_start            // link address (assigned : 0x80000 - 0x20000)
    ldr     w12, =loader_size
    bl      reloacate

_sp_init:
    // set top of stack just before our code
    ldr     x11, =_start
    mov     sp, x11

_bss_init:
    // clear bss segment
    ldr     x10, =_bss_begin
    ldr     x11, =bss_size
    cbz     x11, _main
    bl      memzero

_main:
    adr     x10, main
    ldr     x11, =_offset
    sub     x10, x10, x11
    blr     x10

memzero:
    str     xzr, [x10], #8
    subs    x11, x11, #8
    b.gt    memzero
    ret

reloacate:
    ldr     x13, [x10], #8              // x4 = [x1]; x1 += 8;
    str     x13, [x11], #8              // [x2] = x4; x2 += 8;
    sub     w12, w12, #1
    cbnz    w12, reloacate
    ret