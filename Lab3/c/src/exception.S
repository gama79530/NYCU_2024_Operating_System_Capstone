/*  saved info

    +------------+          sp after saving
    |     x0     |
    +------------+          offset 8 bytes
    |     x1     |
    +------------+
    |      .     | 
    |      .     |
    |      .     |
    +------------+
    |    x29     |
    +------------+          offset 8 * 30 = 240 bytes
    |    x30     |          
    +------------+          sp before saving
*/
.macro stub_wraper handler
    .align 7                            // entry size is 0x80
    bl      mask_interrupt
    str     x30, [sp, #-8]!             // save link register (sp -= 8, store x30)
    bl      save_register_set
    bl      \handler
    bl      restore_register_set
    ldr     x30, [sp], #8               // restore link register (load x30, sp += 8)
    bl      unmask_interrupt
    eret
.endm


/* the structure of vtable_el1

    +------------+       top of vector table should be aligned to 0x800
    |  entry  0  |
    +------------+       offset 0x80
    |  entry  1  |
    +------------+         
    |      .     |
    |      .     |
    |      .     |
    +------------+       
    |  entry 15  |
    +------------+       bottom of vector table
 */
.align 11 // vector table should be aligned to 0x800
.global vtable_el1
vtable_el1:
    // from current EL, using SP_EL0
    stub_wraper  handler_no_op
    stub_wraper  handler_no_op
    stub_wraper  handler_no_op
    stub_wraper  handler_no_op
    // from current EL, using SP_ELx
    stub_wraper  handler_no_op
    stub_wraper  handler_no_op
    stub_wraper  handler_no_op
    stub_wraper  handler_no_op
    // from lower EL, using AArch64
    stub_wraper  handler_type08
    stub_wraper  handler_no_op
    stub_wraper  handler_no_op
    stub_wraper  handler_no_op
    // from lower EL, using AArch32
    stub_wraper  handler_no_op
    stub_wraper  handler_no_op
    stub_wraper  handler_no_op
    stub_wraper  handler_no_op
// End of vtable_el1


.global mask_interrupt
mask_interrupt:
    msr DAIFSet, 0xf
    ret
// End of mask_interrupt


.global unmask_interrupt
unmask_interrupt:
    msr DAIFClr, 0xf
    ret
// End of unmask_interrupt


save_register_set:
    stp     x0, x1, [sp, #-16]!
    stp     x2, x3, [sp, #-16]!
    stp     x4, x5, [sp, #-16]!
    stp     x6, x7, [sp, #-16]!
    stp     x8, x9, [sp, #-16]!
    stp     x10, x11, [sp, #-16]!
    stp     x12, x13, [sp, #-16]!
    stp     x14, x15, [sp, #-16]!
    stp     x16, x17, [sp, #-16]!
    stp     x18, x19, [sp, #-16]!
    stp     x20, x21, [sp, #-16]!
    stp     x22, x23, [sp, #-16]!
    stp     x24, x25, [sp, #-16]!
    stp     x26, x27, [sp, #-16]!
    stp     x28, x29, [sp, #-16]!
    ret
// End of save_register_set


restore_register_set:
    ldp     x28, x29, [sp], #16
    ldp     x26, x27, [sp], #16
    ldp     x24, x25, [sp], #16
    ldp     x22, x23, [sp], #16
    ldp     x20, x21, [sp], #16
    ldp     x18, x19, [sp], #16
    ldp     x16, x17, [sp], #16
    ldp     x14, x15, [sp], #16
    ldp     x12, x13, [sp], #16
    ldp     x10, x11, [sp], #16
    ldp     x8, x9, [sp], #16
    ldp     x6, x7, [sp], #16
    ldp     x4, x5, [sp], #16
    ldp     x2, x3, [sp], #16
    ldp     x0, x1, [sp], #16
    ret
// End of restore_register_set


handler_no_op:
    nop
    ret
// End of handler_no_op


