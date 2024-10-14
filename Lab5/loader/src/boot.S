.section ".text.boot"

.global _start

_start:
    adr     x10, .                  // load address (default  : 0x80000) 
    ldr     x11, =_start            // link address (assigned : 0x80000 - 0x20000)
    ldr     w12, =loader_size
    bl      reloacate

_loader_init:
    // clear bss segment
    ldr     x10, =bss_begin
    ldr     x11, =bss_size
    bl      bss_init

    // set top of stack just before our code
    ldr     x11, =_start
    mov     sp, x11

    // invoke relocated main function
    adr     x10, main
    ldr     x11, =_offset
    sub     x10, x10, x11
    blr     x10


reloacate:
    ldr     x13, [x10], #8              // x4 = [x1]; x1 += 8;
    str     x13, [x11], #8              // [x2] = x4; x2 += 8;
    sub     w12, w12, #1
    cbnz    w12, reloacate
    ret


bss_init:
    cbz     x11, 1f
    str     xzr, [x10], #8
    sub     x11, x11, #1
    b       bss_init
1:
    ret

