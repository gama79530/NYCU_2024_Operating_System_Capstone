#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__
#include "types.h"

/************************************************************************
 * Instructions for accessing the PSTATE fields, page 236 of
 * AArch64-Reference-Manual
 ************************************************************************/
extern void enable_fiq(void);
extern void disable_fiq(void);
extern void enable_irq(void);
extern void disable_irq(void);
extern void enable_serror(void);
extern void disable_serror(void);
extern void enable_debug(void);
extern void disable_debug(void);
extern void enable_all_exception(void);
extern void disable_all_exception(void);
extern void err_hang(void);

typedef void (*irq_task_cb_t)(void);

#endif