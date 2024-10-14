#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__
#include "types.h"

/* Layout of vtable
4 group from base address
    0. from current EL, using SP_EL0
    1. from current EL, using SP_ELx
    2. from lower EL, at least one lower EL is AArch64
    3. from lower EL, all lower ELs are AArch32

for each group
    0. synchronous
    1. IRQ
    2. FIQ
    3. SError

Ex: 
1. entry 13 = 4 * 3 + 1 => IRQ exception & from lower EL, all lower ELs are AArch32
2. entry 4 = 4 * 1 + 0 => synchronous exception & from current EL, using SP_ELx
*/

#define enable_interrupt_all() asm volatile("msr   daifclr, #0xf")
#define disable_interrupt_all() asm volatile("msr   daifset, #0xf")

typedef void (*task_handler_t)(void);

void handler_el1_0(void);
void handler_el1_1(void);
void handler_el1_2(void);
void handler_el1_3(void);
void handler_el1_4(void);
void handler_el1_5(void);   // IRQ exception from current EL, using SP_ELx
void handler_el1_6(void);
void handler_el1_7(void);
void handler_el1_8(void);   // synchronous exception from lower EL, at least one lower EL is AArch64
void handler_el1_9(void);
void handler_el1_10(void);
void handler_el1_11(void);
void handler_el1_12(void);
void handler_el1_13(void);
void handler_el1_14(void);
void handler_el1_15(void);

#endif