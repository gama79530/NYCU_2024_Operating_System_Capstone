#ifndef LAB5_C_TIMER_H
#define LAB5_C_TIMER_H

#include "types.h"

/** Handle expiration; cast args to the type expected by the callback if needed. */
typedef void (*timer_callback_t)(void *args);

/* Initialize the physical timer and route its IRQ to core 0. */
void timer_init(void);

/* Return true when core 0 has a pending physical timer IRQ. */
bool timer_irq_pending(void);

/* Handle a physical timer IRQ and run expired software timer callbacks. */
void timer_handle_irq(void);

/**
 * Add a one-shot timeout; seconds must be positive and callback non-NULL.
 * args may be NULL if the callback needs no arguments. Otherwise, it must point
 * to data that remains valid until the callback finishes using it.
 * Timer stores the pointer without copying or freeing it. On success, the
 * callback is responsible for releasing any owned resources after use;
 * on failure the caller must handle cleanup.
 * Callbacks may run with IRQ enabled; do not yield or exit a thread from them.
 */
bool timer_add_timeout(uint64_t seconds, timer_callback_t callback, void *args);

/* Return the number of seconds since the physical counter started. */
uint64_t timer_current_seconds(void);

#endif
