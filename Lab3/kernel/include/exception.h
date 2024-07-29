#ifndef EXCEPTION_H
#define EXCEPTION_H
#include "types.h"

#define enable_interrupt_all() asm volatile("msr   daifclr, #0xf")
#define disable_interrupt_all() asm volatile("msr   daifset, #0xf")

void handler_irq_current_spelx_el1(void);

void handler_sync_lower_aarch64_el1(void);

#endif