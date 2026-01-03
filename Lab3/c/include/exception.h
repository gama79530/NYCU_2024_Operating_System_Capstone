#ifndef LAB3_C_EXCEPTION_H
#define LAB3_C_EXCEPTION_H
#include "types.h"

extern void mask_interrupt(void);
extern void unmask_interrupt(void);

typedef void (*irq_cb)(void);

#endif