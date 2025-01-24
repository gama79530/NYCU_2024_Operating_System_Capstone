#include "arm_v8.h"
#include "exception.h"
#include "syscall.h"
#include "sched.h"
#include "macro.S"

/*  Because the size of a vector table entry is restricted to 32 words, 
    we use an indirect call to handle exceptions.
*/
.macro ventry label
    .align 7    // entry size is 0x80, ".align" will pad 0
    b \label
.endm

.macro handle_invalid_entry, el, type
    save_state \el
    mov     x0, #\type
    mrs     x1, spsr_el1
    mrs     x2, esr_el1
    mrs     x3, elr_el1
    bl      show_invalid_entry_message
    b       err_hang
.endm

/******************************************************************************************
 * Instructions for accessing the PSTATE fields, page 236 of AArch64-Reference-Manual
 ******************************************************************************************/
.globl enable_fiq
enable_fiq:
    msr     daifclr, #1
    ret

.globl disable_fiq
disable_fiq:
    msr     daifset, #1
    ret

.globl enable_irq
enable_irq:
    msr     daifclr, #2
    ret

.globl disable_irq
disable_irq:
    msr     daifset, #2
    ret

.globl enable_serror
enable_serror:
    msr     daifclr, #4
    ret

.globl disable_serror
disable_serror:
    msr     daifset, #4
    ret

.globl enable_debug
enable_debug:
    msr     daifclr, #8
    ret

.globl disable_debug
disable_debug:
    msr     daifset, #8
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
    handle_invalid_entry 1, SYNC_INVALID_EL1t

handler_el1_1:
    handle_invalid_entry 1, IRQ_INVALID_EL1t

handler_el1_2:
    handle_invalid_entry 1, FIQ_INVALID_EL1t

handler_el1_3:
    handle_invalid_entry 1, ERROR_INVALID_EL1t

handler_el1_4:
    handle_invalid_entry 1, SYNC_INVALID_EL1h

handler_el1_5:
    save_state 1
    bl      irq_el1h
    load_state 1

handler_el1_6:
    handle_invalid_entry 1, FIQ_INVALID_EL1h

handler_el1_7:
    handle_invalid_entry 1, ERROR_INVALID_EL1h

// sync_invalid_el0_64
handler_el1_8:
    save_state 0
    mrs     x25, esr_el1
    lsr     x24, x25, #ESR_ELx_EC_SHIFT
    cmp     x24, #ESR_ELx_EC_SVC64
    b.eq    el0_svc
    handle_invalid_entry 0, SYNC_ERROR

syscall_num     .req    x25
syscall_id      .req    x26
syscall_table   .req    x27
el0_svc:
    mov     syscall_num, #SYS_NUM
    uxtw    syscall_id, w8
    adr     syscall_table, sys_table
    bl      enable_irq
    cmp     syscall_id, syscall_num
    b.hs    invalid_syscall

syscall:
    ldr     x16, [syscall_table, syscall_id, lsl #3]
    blr     x16
    bl      disable_irq
    str     x0, [sp, #STATE_FRAME_OFFSET_X0]
    load_state 0

invalid_syscall:
    handle_invalid_entry 0, SYSCALL_ERROR

.unreq  syscall_num
.unreq  syscall_id
.unreq  syscall_table

handler_el1_9:
    save_state 0
    bl      irq_el0_64
    load_state 0

handler_el1_10:
    handle_invalid_entry 0, FIQ_INVALID_EL0_64

handler_el1_11:
    handle_invalid_entry 0, ERROR_INVALID_EL0_64

handler_el1_12:
    handle_invalid_entry 0, SYNC_INVALID_EL0_32

handler_el1_13:
    handle_invalid_entry 0, IRQ_INVALID_EL0_32

handler_el1_14:
    handle_invalid_entry 0, FIQ_INVALID_EL0_32

handler_el1_15:
    handle_invalid_entry 0, ERROR_INVALID_EL0_32
