.section ".text"

.global memzero
.type memzero, %function
memzero:
    cmp     x1, #8
    b.lt    2f
1:
    str     xzr, [x0], #8
    sub     x1, x1, #8
    cmp     x1, #8
    b.ge    1b
2:
    cbz     x1, 3f
    strb    wzr, [x0], #1
    sub     x1, x1, #1
    cbnz    x1, 2b
3:
    ret
.size memzero, . - memzero

.global get32
.type get32, %function
get32:
    ldr     w0, [x0]
    ret
.size get32, . - get32

.global put32
.type put32, %function
put32:
    str     w1, [x0]
    ret
.size put32, . - put32

.global data_memory_barrier
.type data_memory_barrier, %function
data_memory_barrier:
    dmb     sy
    ret
.size data_memory_barrier, . - data_memory_barrier

.global data_sync_barrier
.type data_sync_barrier, %function
data_sync_barrier:
    dsb     sy
    ret
.size data_sync_barrier, . - data_sync_barrier

.global instruction_sync_barrier
.type instruction_sync_barrier, %function
instruction_sync_barrier:
    isb
    ret
.size instruction_sync_barrier, . - instruction_sync_barrier
