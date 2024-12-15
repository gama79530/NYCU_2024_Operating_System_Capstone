#ifndef __TIMER_H__
#define __TIMER_H__
#include "types.h"

/* You should free the memory space of arg in the callback function if need. */
typedef void (*timer_event_cb_t)(void *arg);

enum time_unit{
    SECOND = 1000000,
    MILLISECOND = 1000,
    MICROSECOND = 1,
};

void core_timer_enable(void);
void core_timer_disable(void);
void timer_set_countdown(uint64_t countdown, enum time_unit unit);

uint64_t timer_get_current_time(enum time_unit unit);
void timer_add_timeout_event(enum time_unit unit, uint64_t countdown, uint64_t period, timer_event_cb_t callback, void *arg);
void irq_timer_event(void);

#endif