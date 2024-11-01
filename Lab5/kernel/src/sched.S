#include "sched.h"

.globl thread_start
thread_start:
	bl      preemption_enable
	mov     x0, x20
	blr	    x19
    bl      thread_exit
    // should never return

.macro _save_context reg
	add		x8, \reg, #CPU_CONTEXT_OFFSET
	mov		x9, sp
	stp		x19, x20, [x8], #16
	stp		x21, x22, [x8], #16
	stp		x23, x24, [x8], #16
	stp		x25, x26, [x8], #16
	stp		x27, x28, [x8], #16
	stp		x29, x30, [x8], #16
	str		x9, [x8]
.endm

.macro _load_context reg
	add		x8, \reg, #CPU_CONTEXT_OFFSET
	ldp		x19, x20, [x8], #16
	ldp		x21, x22, [x8], #16
	ldp		x23, x24, [x8], #16
	ldp		x25, x26, [x8], #16
	ldp		x27, x28, [x8], #16
	ldp		x29, x30, [x8], #16
	ldr		x9, [x8]
	mov		sp, x9
.endm

.globl context_switch
context_switch:
	_save_context x0
	_load_context x1
	ret

.globl context_load
context_load:
	_load_context x0
	ret