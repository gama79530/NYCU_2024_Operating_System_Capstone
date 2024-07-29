#ifndef TIMER_H
#define TIMER_H
#include "types.h"


/*  Core interrupt source
    ref: https://github.com/Tekki/raspberrypi-documentation/blob/master/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf
         section 4.10, page 16
*/
#define CNTPNSIRQ           (1 << 1)

enum tick{
    SECOND = 1,
    MILLISECOND = 1000,
    MICROSECOND = 1000000
};

void core_timer_enable(void);
void core_timer_disable(void);
void set_period(uint64_t period, enum tick unit);

uint64_t get_current_time(void);

#endif