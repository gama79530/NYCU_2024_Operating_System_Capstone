.global get_cpu_id
.type get_cpu_id, %function
get_cpu_id:
    mrs     x0, mpidr_el1
    and     x0, x0, #0xff
    ret

.global memzero
.type memzero, %function
memzero:
    cmp     x1, #8
    b.lt    2f
1:  // set 8 bytes
    str     xzr, [x0], #8
    sub     x1, x1, #8
    cmp     x1, #8
    b.ge    1b
2:  // set 1 byte
    cbz     x1, 3f
    strb    wzr, [x0], #1
    sub     x1, x1, #1
    cbnz    x1, 2b
3:  
    ret

.global get32
.type get32, %function
get32:
    ldr     w0, [x0]
    ret

.global put32
.type put32, %function
put32:
    str w1, [x0]
    ret
