#ifndef __TIMER_H__
#define __TIMER_H__
#include "types.h"

/* You should free the memory space of arg in the callback function if need. */
typedef void (*timer_event_cb_t)(void *arg);

typedef enum time_unit{
    SECOND = 1L,
    MILLISECOND = 1000L,
    MICROSECOND = 1000000L,
} time_unit_t;

uint64_t get_count_frequency(void);
uint64_t get_physical_count(void);
uint64_t timer_get_time(time_unit_t unit);

uint64_t time_to_count(time_unit_t unit, uint64_t time);
uint64_t count_to_time(time_unit_t unit, uint64_t count);

int timer_init(void);
void enable_core0_timer(void);
void disable_core0_timer(void);

void timer_set_countdown(uint64_t countdown);
void timer_add_timeout_event(
    time_unit_t unit,
    uint64_t time,
    uint64_t period,
    timer_event_cb_t callback,
    void *arg
);
void timer_add_countdown_event(
    uint64_t countdown,
    uint64_t period_count,
    timer_event_cb_t callback,
    void *arg
);
void irq_timer_event(void);

void enable_time_sharing(void);
void disable_time_sharing(void);
bool get_time_sharing_flag(void);
void set_time_sharing_period(uint64_t period_count);

#endif