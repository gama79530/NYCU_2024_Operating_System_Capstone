/*  Because the size of a vector table entry is restricted to 32 words, 
    we use an indirect call to handle exceptions.
*/ 
.macro ventry label
    .align 7    // entry size is 0x80, .align will pad 0
    b \label
.endm

// save general registers to stack
.macro save_all
    sub     sp, sp, 32 * 8
    stp     x0, x1, [sp ,16 * 0]
    stp     x2, x3, [sp ,16 * 1]
    stp     x4, x5, [sp ,16 * 2]
    stp     x6, x7, [sp ,16 * 3]
    stp     x8, x9, [sp ,16 * 4]
    stp     x10, x11, [sp ,16 * 5]
    stp     x12, x13, [sp ,16 * 6]
    stp     x14, x15, [sp ,16 * 7]
    stp     x16, x17, [sp ,16 * 8]
    stp     x18, x19, [sp ,16 * 9]
    stp     x20, x21, [sp ,16 * 10]
    stp     x22, x23, [sp ,16 * 11]
    stp     x24, x25, [sp ,16 * 12]
    stp     x26, x27, [sp ,16 * 13]
    stp     x28, x29, [sp ,16 * 14]
    str     x30, [sp, 16 * 15]
.endm

// load general registers from stack
.macro load_all
    ldp     x0, x1, [sp ,16 * 0]
    ldp     x2, x3, [sp ,16 * 1]
    ldp     x4, x5, [sp ,16 * 2]
    ldp     x6, x7, [sp ,16 * 3]
    ldp     x8, x9, [sp ,16 * 4]
    ldp     x10, x11, [sp ,16 * 5]
    ldp     x12, x13, [sp ,16 * 6]
    ldp     x14, x15, [sp ,16 * 7]
    ldp     x16, x17, [sp ,16 * 8]
    ldp     x18, x19, [sp ,16 * 9]
    ldp     x20, x21, [sp ,16 * 10]
    ldp     x22, x23, [sp ,16 * 11]
    ldp     x24, x25, [sp ,16 * 12]
    ldp     x26, x27, [sp ,16 * 13]
    ldp     x28, x29, [sp ,16 * 14]
    ldr     x30, [sp, 16 * 15]
    add     sp, sp, 32 * 8
.endm

.macro entry_wraper entry_name handler_name
\entry_name:
    msr daifset, #0xf
    save_all
    bl  \handler_name
    load_all
    msr daifclr, #0xf
    eret
.endm

.align 11 // vector table should be aligned to 0x800
.global vtable_el1
vtable_el1:
    // from current EL, using SP_EL0
    ventry  entry_el1_0
    ventry  entry_el1_1
    ventry  entry_el1_2
    ventry  entry_el1_3
    // from current EL, using SP_ELx
    ventry  entry_el1_4
    ventry  entry_el1_5
    ventry  entry_el1_6
    ventry  entry_el1_7
    // from lower EL, at least one lower EL is AArch64
    ventry  entry_el1_8
    ventry  entry_el1_9
    ventry  entry_el1_10
    ventry  entry_el1_11
    // from lower EL, all lower ELs are AArch32
    ventry  entry_el1_12
    ventry  entry_el1_13
    ventry  entry_el1_14
    ventry  entry_el1_15

entry_wraper entry_el1_0 handler_el1_0

entry_wraper entry_el1_1 handler_el1_1

entry_wraper entry_el1_2 handler_el1_2

entry_wraper entry_el1_3 handler_el1_3

entry_wraper entry_el1_4 handler_el1_4

entry_wraper entry_el1_5 handler_el1_5

entry_wraper entry_el1_6 handler_el1_6

entry_wraper entry_el1_7 handler_el1_7

entry_wraper entry_el1_8 handler_el1_8

entry_wraper entry_el1_9 handler_el1_9

entry_wraper entry_el1_10 handler_el1_10

entry_wraper entry_el1_11 handler_el1_11

entry_wraper entry_el1_12 handler_el1_12

entry_wraper entry_el1_13 handler_el1_13

entry_wraper entry_el1_14 handler_el1_14

entry_wraper entry_el1_15 handler_el1_15

/*
handler_no_op:
    nop

handler_wrapper handler_no_op

handler_wrapper handler_sync_lower_aarch64_el1

handler_wrapper handler_irq_current_spelx_el1
*/
