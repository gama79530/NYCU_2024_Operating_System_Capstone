#ifndef __TIMER_H__
#define __TIMER_H__
#include "types.h"

/*  Core interrupt source
    ref: https://github.com/Tekki/raspberrypi-documentation/blob/master/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf
         section 4.10, page 16
*/
#define CNTPNSIRQ           (1 << 1)

/* You should free the memory space of arg in the callback function if need. */
typedef void (*timer_event_callback_t)(void *arg);

enum time_unit{
    SECOND = 1000000,
    MILLISECOND = 1000,
    MICROSECOND = 1,
};

void core_timer_enable(void);
void core_timer_disable(void);
void timer_set_countdown(uint64_t countdown, enum time_unit unit);

uint64_t timer_get_current_time(enum time_unit unit);
void timer_add_timeout_event(uint64_t countdown, enum time_unit unit, timer_event_callback_t cb, void *arg);
void handler_el1_5_timer_event(void);

#endif