#include "config.h"
/*  Because the size of a vector table entry is restricted to 32 words, 
    we use an indirect call to handle exceptions.
*/
.macro ventry label
    .align 7    // entry size is 0x80, .align will pad 0
    b \label
.endm

// save general registers to stack
.macro save_reg, el
    sub     sp, sp, #S_FRAME_SIZE
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
    mrs     x21, sp_el0
.else
    add     x21, sp, #S_FRAME_SIZE
.endif
    stp     x30, x21, [sp, 16 * 15]
    // push information for nested interrupt
    mrs     x22, spsr_el1
    mrs     x23, elr_el1   
    stp     x22, x23, [sp, 16 * 16]
.endm

// load general registers from stack
.macro load_reg, el
    // pop information for nested interrupt
    ldp     x22, x23, [sp, 16 * 16]
    msr     spsr_el1, x22
    msr     elr_el1, x23   
    ldp     x30, x21, [sp, 16 * 15]
.if \el == 0
    msr     sp_el0, x21
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
    add     sp, sp, #S_FRAME_SIZE
    eret
.endm

.macro handle_invalid_entry, el, type
    save_reg \el
    mov     x0, #\type
    mrs     x1, spsr_el1
    mrs     x2, esr_el1
    mrs     x3, elr_el1
    bl      show_invalid_entry_message
    b       err_hang
.endm

/************************************************************************
 * Instructions for accessing the PSTATE fields, page 236 of
 * AArch64-Reference-Manual
 ************************************************************************/
.globl enable_fiq
enable_fiq:
    msr daifclr, #1
    ret

.globl disable_fiq
disable_fiq:
    msr daifset, #1
    ret

.globl enable_irq
enable_irq:
    msr daifclr, #2
    ret

.globl disable_irq
disable_irq:
    msr daifset, #2
    ret

.globl enable_serror
enable_serror:
    msr daifclr, #4
    ret

.globl disable_serror
disable_serror:
    msr daifset, #4
    ret

.globl enable_debug
enable_debug:
    msr daifclr, #8
    ret

.globl disable_debug
disable_debug:
    msr daifset, #8
    ret

.globl enable_all_exception
enable_all_exception:
    msr     daifclr, #0xf
    ret

.globl disable_all_exception
disable_all_exception:
    msr     daifset, #0xf
    ret

.globl err_hang
err_hang:
    b       err_hang

/*****************************************************************************************
 * Layout of vtable
 * 4 group from base address
 *  0. from current EL, using SP_EL0
 *  1. from current EL, using SP_ELx
 *  2. from lower EL, at least one lower EL is AArch64
 *  3. from lower EL, all lower ELs are AArch32
 * 
 * for each group
 *  0. synchronous
 *  1. IRQ
 *  2. FIQ
 *  3. SError
 * 
 * Ex:
 *  1. entry 13 = 4 * 3 + 1 => IRQ exception & from lower EL, all lower ELs are AArch32
 *  2. entry 4 = 4 * 1 + 0 => synchronous exception & from current EL, using SP_ELx
 *****************************************************************************************/
.align 11 // vector table should be aligned to 0x800
.global vtable_el1
vtable_el1:
    ventry  handler_el1_0
    ventry  handler_el1_1
    ventry  handler_el1_2
    ventry  handler_el1_3
    ventry  handler_el1_4
    ventry  handler_el1_5
    ventry  handler_el1_6
    ventry  handler_el1_7
    ventry  handler_el1_8
    ventry  handler_el1_9
    ventry  handler_el1_10
    ventry  handler_el1_11
    ventry  handler_el1_12
    ventry  handler_el1_13
    ventry  handler_el1_14
    ventry  handler_el1_15

handler_el1_0:
    handle_invalid_entry 1, 0

handler_el1_1:
    handle_invalid_entry 1, 1

handler_el1_2:
    handle_invalid_entry 1, 2

handler_el1_3:
    handle_invalid_entry 1, 3

handler_el1_4:
    handle_invalid_entry 1, 4

handler_el1_5:
    save_reg 1
    bl  irq_invalid_el1h
    load_reg 1

handler_el1_6:
    handle_invalid_entry 1, 6

handler_el1_7:
    handle_invalid_entry 1, 7

// sync_invalid_el0_64
handler_el1_8:
    save_reg 0
    b err_hang
/*
  save_all 0
  mrs x25, esr_el1
  lsr x24, x25, #ESR_ELx_EC_SHIFT
  cmp x24, #ESR_ELx_EC_SVC64
  b.eq el0_svc
  handle_invalid_entry 0 SYNC_ERROR

scnr .req x25
scno .req x26
stbl .req x27

el0_svc:
  adr stbl, sys_call_table
  uxtw scno, w8
  mov scnr, #NR_SYSCALLS
  bl enable_irq
  cmp scno, scnr
  b.hs ni_sys

  ldr x16, [stbl, scno, lsl #3]
  blr x16
  b ret_from_syscall
  */
// temp

handler_el1_9:
    save_reg 0
    bl  irq_invalid_el0_64
    load_reg 0

handler_el1_10:
    handle_invalid_entry 0, 10

handler_el1_11:
    handle_invalid_entry 0, 11

handler_el1_12:
    handle_invalid_entry 0, 12

handler_el1_13:
    handle_invalid_entry 0, 13

handler_el1_14:
    handle_invalid_entry 0, 14

handler_el1_15:
    handle_invalid_entry 0, 15

