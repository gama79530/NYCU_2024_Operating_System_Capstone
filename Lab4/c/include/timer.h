#ifndef LAB4_C_TIMER_H
#define LAB4_C_TIMER_H

#include "types.h"

/* Callback invoked when a software timer expires. */
typedef void (*timer_callback_t)(const char *message, uint64_t current_seconds);

/* Initialize the physical timer and route its IRQ to core 0. */
void timer_init(void);

/* Return true when core 0 has a pending physical timer IRQ. */
bool timer_irq_pending(void);

/* Handle a physical timer IRQ and run expired software timer callbacks. */
void timer_handle_irq(void);

/* Add a one-shot timeout that expires after the given number of seconds. */
bool timer_add_timeout(uint64_t seconds, const char *message, timer_callback_t callback);

/* Return the number of seconds since the physical counter started. */
uint64_t timer_current_seconds(void);

#endif
