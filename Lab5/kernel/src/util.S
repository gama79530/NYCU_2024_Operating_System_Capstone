.global get_cpu_id
get_cpu_id:
    mrs     x0, mpidr_el1
    and     x0, x0, #0xff
    ret

.global get_current_el
get_current_el:
    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    ret

.global memzero
memzero:
    cmp     x1, #8
    b.lt    1f
    str     xzr, [x0], #8
    sub     x1, x1, #8
    b       memzero
1:
    cbz     x1, 1f
    strb    wzr, [x0], #1
    sub     x1, x1, #1
    b       1b
1:
    ret

.global memcpy
memcpy:
    cmp     x2, #8
    b.lt    1f
    ldr     x9, [x1], #8
    str     x9, [x0], #8
    sub     x2, x2, #8
    b       memcpy
1:
    cbz     x2, 1f
    ldrb    w9, [x1], #1
    strb    w9, [x0], #1
    sub     x2, x2, #1
    b       1b
1:
    ret