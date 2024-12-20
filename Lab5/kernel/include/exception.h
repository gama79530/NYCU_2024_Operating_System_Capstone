#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

/* vector table id */
#define SYNC_INVALID_EL1t       0
#define IRQ_INVALID_EL1t        1
#define FIQ_INVALID_EL1t        2
#define ERROR_INVALID_EL1t      3
#define SYNC_INVALID_EL1h       4
#define IRQ_INVALID_EL1h        5
#define FIQ_INVALID_EL1h        6
#define ERROR_INVALID_EL1h      7
#define SYNC_INVALID_EL0_64     8
#define IRQ_INVALID_EL0_64      9
#define FIQ_INVALID_EL0_64      10
#define ERROR_INVALID_EL0_64    11
#define SYNC_INVALID_EL0_32     12
#define IRQ_INVALID_EL0_32      13
#define FIQ_INVALID_EL0_32      14
#define ERROR_INVALID_EL0_32    15
#define SYNC_ERROR              16
#define SYSCALL_ERROR           17

#ifndef __ASSEMBLER__
#include "types.h"

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
#endif