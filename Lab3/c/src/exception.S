#include "exception.h"

#define EXCEPTION_FRAME_SIZE 272
#define EXCEPTION_FRAME_REGS_0_1 0
#define EXCEPTION_FRAME_REGS_2_3 16
#define EXCEPTION_FRAME_REGS_4_5 32
#define EXCEPTION_FRAME_REGS_6_7 48
#define EXCEPTION_FRAME_REGS_8_9 64
#define EXCEPTION_FRAME_REGS_10_11 80
#define EXCEPTION_FRAME_REGS_12_13 96
#define EXCEPTION_FRAME_REGS_14_15 112
#define EXCEPTION_FRAME_REGS_16_17 128
#define EXCEPTION_FRAME_REGS_18_19 144
#define EXCEPTION_FRAME_REGS_20_21 160
#define EXCEPTION_FRAME_REGS_22_23 176
#define EXCEPTION_FRAME_REGS_24_25 192
#define EXCEPTION_FRAME_REGS_26_27 208
#define EXCEPTION_FRAME_REGS_28_29 224
#define EXCEPTION_FRAME_REGS_30_RESERVED 240
#define EXCEPTION_FRAME_SPSR_EL1_ELR_EL1 256

.section ".text"

.macro vector_entry target
    .align 7
    b       \target
.endm

.macro save_exception_frame
    msr     daifset, #0xf
    sub     sp, sp, #EXCEPTION_FRAME_SIZE

    stp     x0, x1, [sp, #EXCEPTION_FRAME_REGS_0_1]
    stp     x2, x3, [sp, #EXCEPTION_FRAME_REGS_2_3]
    stp     x4, x5, [sp, #EXCEPTION_FRAME_REGS_4_5]
    stp     x6, x7, [sp, #EXCEPTION_FRAME_REGS_6_7]
    stp     x8, x9, [sp, #EXCEPTION_FRAME_REGS_8_9]
    stp     x10, x11, [sp, #EXCEPTION_FRAME_REGS_10_11]
    stp     x12, x13, [sp, #EXCEPTION_FRAME_REGS_12_13]
    stp     x14, x15, [sp, #EXCEPTION_FRAME_REGS_14_15]
    stp     x16, x17, [sp, #EXCEPTION_FRAME_REGS_16_17]
    stp     x18, x19, [sp, #EXCEPTION_FRAME_REGS_18_19]
    stp     x20, x21, [sp, #EXCEPTION_FRAME_REGS_20_21]
    stp     x22, x23, [sp, #EXCEPTION_FRAME_REGS_22_23]
    stp     x24, x25, [sp, #EXCEPTION_FRAME_REGS_24_25]
    stp     x26, x27, [sp, #EXCEPTION_FRAME_REGS_26_27]
    stp     x28, x29, [sp, #EXCEPTION_FRAME_REGS_28_29]
    stp     x30, xzr, [sp, #EXCEPTION_FRAME_REGS_30_RESERVED]

    mrs     x0, spsr_el1
    mrs     x1, elr_el1
    stp     x0, x1, [sp, #EXCEPTION_FRAME_SPSR_EL1_ELR_EL1]
.endm

.macro exception_entry label, origin, class
\label:
    save_exception_frame
    mov     x0, sp
    mov     x1, #\origin
    mov     x2, #\class
    bl      exception_dispatch
    b       restore_exception_frame
.endm

.align 11
.global vector_table_el1
.type vector_table_el1, %function
vector_table_el1:
    // Current EL with SP_EL0.
    vector_entry current_sp0_sync_entry
    vector_entry current_sp0_irq_entry
    vector_entry current_sp0_fiq_entry
    vector_entry current_sp0_serror_entry

    // Current EL with SP_ELx.
    vector_entry current_spx_sync_entry
    vector_entry current_spx_irq_entry
    vector_entry current_spx_fiq_entry
    vector_entry current_spx_serror_entry

    // Lower EL using AArch64.
    vector_entry lower_aarch64_sync_entry
    vector_entry lower_aarch64_irq_entry
    vector_entry lower_aarch64_fiq_entry
    vector_entry lower_aarch64_serror_entry

    // Lower EL using AArch32.
    vector_entry lower_aarch32_sync_entry
    vector_entry lower_aarch32_irq_entry
    vector_entry lower_aarch32_fiq_entry
    vector_entry lower_aarch32_serror_entry
.size vector_table_el1, . - vector_table_el1

exception_entry current_sp0_sync_entry, EXCEPTION_ORIGIN_CURRENT_SP0_VALUE, EXCEPTION_CLASS_SYNC_VALUE
exception_entry current_sp0_irq_entry, EXCEPTION_ORIGIN_CURRENT_SP0_VALUE, EXCEPTION_CLASS_IRQ_VALUE
exception_entry current_sp0_fiq_entry, EXCEPTION_ORIGIN_CURRENT_SP0_VALUE, EXCEPTION_CLASS_FIQ_VALUE
exception_entry current_sp0_serror_entry, EXCEPTION_ORIGIN_CURRENT_SP0_VALUE, EXCEPTION_CLASS_SERROR_VALUE

exception_entry current_spx_sync_entry, EXCEPTION_ORIGIN_CURRENT_SPX_VALUE, EXCEPTION_CLASS_SYNC_VALUE
exception_entry current_spx_irq_entry, EXCEPTION_ORIGIN_CURRENT_SPX_VALUE, EXCEPTION_CLASS_IRQ_VALUE
exception_entry current_spx_fiq_entry, EXCEPTION_ORIGIN_CURRENT_SPX_VALUE, EXCEPTION_CLASS_FIQ_VALUE
exception_entry current_spx_serror_entry, EXCEPTION_ORIGIN_CURRENT_SPX_VALUE, EXCEPTION_CLASS_SERROR_VALUE

exception_entry lower_aarch64_sync_entry, EXCEPTION_ORIGIN_LOWER_AARCH64_VALUE, EXCEPTION_CLASS_SYNC_VALUE
exception_entry lower_aarch64_irq_entry, EXCEPTION_ORIGIN_LOWER_AARCH64_VALUE, EXCEPTION_CLASS_IRQ_VALUE
exception_entry lower_aarch64_fiq_entry, EXCEPTION_ORIGIN_LOWER_AARCH64_VALUE, EXCEPTION_CLASS_FIQ_VALUE
exception_entry lower_aarch64_serror_entry, EXCEPTION_ORIGIN_LOWER_AARCH64_VALUE, EXCEPTION_CLASS_SERROR_VALUE

exception_entry lower_aarch32_sync_entry, EXCEPTION_ORIGIN_LOWER_AARCH32_VALUE, EXCEPTION_CLASS_SYNC_VALUE
exception_entry lower_aarch32_irq_entry, EXCEPTION_ORIGIN_LOWER_AARCH32_VALUE, EXCEPTION_CLASS_IRQ_VALUE
exception_entry lower_aarch32_fiq_entry, EXCEPTION_ORIGIN_LOWER_AARCH32_VALUE, EXCEPTION_CLASS_FIQ_VALUE
exception_entry lower_aarch32_serror_entry, EXCEPTION_ORIGIN_LOWER_AARCH32_VALUE, EXCEPTION_CLASS_SERROR_VALUE

restore_exception_frame:
    ldp     x0, x1, [sp, #EXCEPTION_FRAME_SPSR_EL1_ELR_EL1]
    msr     spsr_el1, x0
    msr     elr_el1, x1

    ldp     x30, xzr, [sp, #EXCEPTION_FRAME_REGS_30_RESERVED]
    ldp     x28, x29, [sp, #EXCEPTION_FRAME_REGS_28_29]
    ldp     x26, x27, [sp, #EXCEPTION_FRAME_REGS_26_27]
    ldp     x24, x25, [sp, #EXCEPTION_FRAME_REGS_24_25]
    ldp     x22, x23, [sp, #EXCEPTION_FRAME_REGS_22_23]
    ldp     x20, x21, [sp, #EXCEPTION_FRAME_REGS_20_21]
    ldp     x18, x19, [sp, #EXCEPTION_FRAME_REGS_18_19]
    ldp     x16, x17, [sp, #EXCEPTION_FRAME_REGS_16_17]
    ldp     x14, x15, [sp, #EXCEPTION_FRAME_REGS_14_15]
    ldp     x12, x13, [sp, #EXCEPTION_FRAME_REGS_12_13]
    ldp     x10, x11, [sp, #EXCEPTION_FRAME_REGS_10_11]
    ldp     x8, x9, [sp, #EXCEPTION_FRAME_REGS_8_9]
    ldp     x6, x7, [sp, #EXCEPTION_FRAME_REGS_6_7]
    ldp     x4, x5, [sp, #EXCEPTION_FRAME_REGS_4_5]
    ldp     x2, x3, [sp, #EXCEPTION_FRAME_REGS_2_3]
    ldp     x0, x1, [sp, #EXCEPTION_FRAME_REGS_0_1]

    add     sp, sp, #EXCEPTION_FRAME_SIZE
    eret
