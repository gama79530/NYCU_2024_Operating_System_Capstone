#ifndef TIMER_H
#define TIMER_H
#include "types.h"

#define MSG_LEN_LIMIT   88

/*  Core interrupt source
    ref: https://github.com/Tekki/raspberrypi-documentation/blob/master/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf
         section 4.10, page 16
*/
#define CNTPNSIRQ           (1 << 1)

typedef void (*timer_event_callback_t)(const char* msg);

enum tick{
    SECOND = 1,
    MILLISECOND = 1000,
    MICROSECOND = 1000000
};

void core_timer_enable(void);
void core_timer_disable(void);
void timer_set_period(uint64_t period, enum tick unit);

uint64_t timer_get_current_time(void);
void timer_add_timeout_event(uint64_t duration, const char *msg);

void handler_irq_timer_event(void);

#endif