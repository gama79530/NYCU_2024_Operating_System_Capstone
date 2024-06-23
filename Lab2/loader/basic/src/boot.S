.section ".text.boot"

.global _start

_start:
    // set top of stack just before our code
    ldr     x11, =_start
    mov     sp, x11

    // clear bss segment
    ldr     x10, =_bss_begin
    ldr     x11, =bss_size
    cbz     x11, _main
    bl      reset_bss

_main:
    bl      main

reset_bss:
    str     xzr, [x10], #8
    subs    x11, x11, #8
    b.gt    reset_bss
    ret

