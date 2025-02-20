#include "sched.h"

// save general registers to stack
.macro save_state, el
    sub     sp, sp, #STATE_FRAME_SIZE
    stp     x0, x1, [sp, 16 * 0]
    stp     x2, x3, [sp, 16 * 1]
    stp     x4, x5, [sp, 16 * 2]
    stp     x6, x7, [sp, 16 * 3]
    stp     x8, x9, [sp, 16 * 4]
    stp     x10, x11, [sp, 16 * 5]
    stp     x12, x13, [sp, 16 * 6]
    stp     x14, x15, [sp, 16 * 7]
    stp     x16, x17, [sp, 16 * 8]
    stp     x18, x19, [sp, 16 * 9]
    stp     x20, x21, [sp, 16 * 10]
    stp     x22, x23, [sp, 16 * 11]
    stp     x24, x25, [sp, 16 * 12]
    stp     x26, x27, [sp, 16 * 13]
    stp     x28, x29, [sp, 16 * 14]
.if \el == 0
    mrs     x22, sp_el0
.else
    add     x22, sp, #STATE_FRAME_SIZE
.endif
    stp     x30, x22, [sp, 16 * 15]
    // push information for nested interrupt
    mrs     x23, spsr_el1
    mrs     x24, elr_el1   
    stp     x23, x24, [sp, 16 * 16]
.endm

// load general registers from stack
.macro load_state, el
    // pop information for nested interrupt
    ldp     x23, x24, [sp, 16 * 16]
    msr     spsr_el1, x23
    msr     elr_el1, x24   
    ldp     x30, x22, [sp, 16 * 15]
.if \el == 0
    msr     sp_el0, x22
.endif
    ldp     x28, x29, [sp, 16 * 14]
    ldp     x26, x27, [sp, 16 * 13]
    ldp     x24, x25, [sp, 16 * 12]
    ldp     x22, x23, [sp, 16 * 11]
    ldp     x20, x21, [sp, 16 * 10]
    ldp     x18, x19, [sp, 16 * 9]
    ldp     x16, x17, [sp, 16 * 8]
    ldp     x14, x15, [sp, 16 * 7]
    ldp     x12, x13, [sp, 16 * 6]
    ldp     x10, x11, [sp, 16 * 5]
    ldp     x8, x9, [sp, 16 * 4]
    ldp     x6, x7, [sp, 16 * 3]
    ldp     x4, x5, [sp, 16 * 2]
    ldp     x2, x3, [sp, 16 * 1]
    ldp     x0, x1, [sp, 16 * 0]
    add     sp, sp, #STATE_FRAME_SIZE
    eret
.endm
