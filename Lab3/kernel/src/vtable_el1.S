// Because the size of a vector table entry is restricted to 32 words, we use an indirect call to handle exceptions.
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

.macro handler_wrapper handler
_\handler:
    msr daifset, #0xf
    save_all
    bl  \handler
    load_all
    msr daifclr, #0xf
    eret
.endm

.align 11 // vector table should be aligned to 0x800
.global vtable_el1
vtable_el1:
    // from current EL, using SP_EL0
    ventry  _handler_no_op
    ventry  _handler_no_op
    ventry  _handler_no_op
    ventry  _handler_no_op
    // from current EL, using SP_ELx
    ventry  _handler_no_op
    ventry  _handler_irq_current_spelx_el1
    ventry  _handler_no_op
    ventry  _handler_no_op
    // from lower EL, using AArch64
    ventry  _handler_sync_lower_aarch64_el1
    ventry  _handler_no_op
    ventry  _handler_no_op
    ventry  _handler_no_op
    // from lower EL, using AArch32
    ventry  _handler_no_op
    ventry  _handler_no_op
    ventry  _handler_no_op
    ventry  _handler_no_op

handler_no_op:
    nop

handler_wrapper handler_no_op

handler_wrapper handler_sync_lower_aarch64_el1

handler_wrapper handler_irq_current_spelx_el1

