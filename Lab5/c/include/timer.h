#ifndef LAB5_C_TIMER_H
#define LAB5_C_TIMER_H

#include "types.h"

typedef enum {
    TIMER_UNIT_TICKS,
    TIMER_UNIT_MILLISECONDS,
    TIMER_UNIT_SECONDS,
} timer_time_unit_t;

/** Handle expiration; cast args to the type expected by the callback if needed. */
typedef void (*timer_callback_t)(void *args);

/** Initialize the ID pool and physical timer; return false on failure.
 * Call once during boot after allocator setup, before enabling IRQ. */
bool timer_init(void);

/* Return true when core 0 has a pending physical timer IRQ. */
bool timer_irq_pending(void);

/* Handle a physical timer IRQ and run expired software timer callbacks. */
void timer_handle_irq(void);

/**
 * Add a timeout; return a reusable ID >= 2, or -1 on failure.
 * duration must be positive and callback non-NULL; milliseconds round up to ticks.
 * Periodic events wait duration again after each callback returns (no catch-up).
 * An overflowing periodic deadline saturates at UINT64_MAX.
 * args may be NULL. Otherwise it must be the original pointer to one contiguous
 * allocator allocation that can be released with a single free(args). Package
 * all event-owned argument data in that allocation; separately allocated nested
 * data is not freed recursively. Do not pass stack/static storage, an interior
 * pointer, or an allocation owned by another event.
 * args is not copied. On success, timer takes ownership and frees it exactly once
 * after the final callback returns. Neither caller
 * nor callback may free it. On add failure, ownership remains with the caller.
 * Callbacks run with IRQ enabled; do not yield or exit a thread from them.
 * IDs must not be used after final completion, since they can be reused.
 */
int timer_add_timeout(timer_time_unit_t unit, uint64_t duration, bool periodic,
                      timer_callback_t callback, void *args);

/**
 * Stop repeating after the pending/current invocation; return false if not found.
 * A waiting event keeps its deadline and still runs once; a running event finishes
 * its current callback. Both are freed with args after that final callback returns.
 * One-shots are unchanged but return true when found. Repeated calls also succeed
 * until completion. This does not invoke a callback or free anything immediately;
 * long delays retain resources until the deadline, and callbacks must return.
 */
bool timer_stop_periodic(int id);

/** Return ticks per second, or zero before successful initialization. */
uint64_t timer_counter_frequency(void);

/* Return the number of seconds since the physical counter started. */
uint64_t timer_current_seconds(void);

#endif
