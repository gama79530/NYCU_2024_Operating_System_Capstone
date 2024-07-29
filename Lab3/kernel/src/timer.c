#include "timer.h"
#include "peripheral.h"

void core_timer_enable(void){
    // enable timer
    asm volatile(
        "mov    x0, 1\n"
        "msr    cntp_ctl_el0, x0\n"
        :
        :
        : "x0"
    );
    // set signal route
    asm volatile(
        "mov    x0, 2\n"
        "ldr    x1, =%0\n"
        "str    w0, [x1]\n"
        :
        : "i"(CORE0_TIMER_IRQCNTL)
        : "x0", "x1"
    );
}

void core_timer_disable(void){
    // disable timer
    asm volatile(
        "mov    x0, 0\n"
        "msr    cntp_ctl_el0, x0\n"
        :
        :
        : "x0"
    );
}

void set_period(uint64_t period, enum tick unit){
    uint64_t tick;
    
    asm volatile("mrs   %0, cntfrq_el0": "=r"(tick));
    tick = tick * period / unit;
    asm volatile("msr    cntp_tval_el0, %0":: "r"(tick));
}

uint64_t get_current_time(void){
    uint64_t physical_count;
    uint64_t freq;
    asm volatile(
        "mrs   %0, cntpct_el0\n"
        "mrs   %1, cntfrq_el0\n"
        : "=r"(physical_count), "=r"(freq)
    );

    return physical_count / freq;
}