#include "timer.h"
#include "peripheral.h"
#include "list.h"
#include "memory.h"
#include "exception.h"
#include "util.h"

static LIST_HEAD(waiting_event_q);

typedef struct timer_event{
    list_head_t             head;
    uint64_t                registration_time;
    uint64_t                expired_time;
    uint64_t                period;
    timer_event_cb_t        callback;
    void                    *arg;
} timer_event_t;

void add_waiting_event(timer_event_t *event);

void core_timer_enable(void){
    /********************************************************************
     * enable timer
     * Page 2179 of AArch64-Reference-Manual
     ********************************************************************/
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
    /********************************************************************
     * disable timer
     * Page 2179 of AArch64-Reference-Manual
     ********************************************************************/
    asm volatile(
        "mov    x0, 0\n"
        "msr    cntp_ctl_el0, x0\n"
        :
        :
        : "x0"
    );
}

void timer_set_countdown(uint64_t countdown, time_unit_t unit){
    uint64_t tick;
    asm volatile("mrs   %0, cntfrq_el0": "=r"(tick));
    tick *= countdown;

    if(unit == MILLISECOND)
        tick /= 1000;
    else if(unit == MICROSECOND)
        tick /= 1000000;

    asm volatile("msr    cntp_tval_el0, %0":: "r"(tick));
}

uint64_t timer_get_current_time(time_unit_t unit){
    uint64_t physical_count, freq, current_time;
    asm volatile(
        "mrs   %0, cntpct_el0\n"
        "mrs   %1, cntfrq_el0\n"
        : "=r"(physical_count), "=r"(freq)
    );

    current_time = physical_count / (freq * unit / 1000000);

    return current_time;
}

void timer_add_timeout_event(time_unit_t unit, uint64_t countdown, uint64_t period, timer_event_cb_t callback, void *arg){
    timer_event_t *event = NULL;
    if(countdown == 0){
        return;
    }
    event = (timer_event_t*)malloc(sizeof(timer_event_t));
    if(event == NULL){
        return;
    }
    event->registration_time = timer_get_current_time(MICROSECOND);
    event->expired_time = event->registration_time + countdown * unit;
    event->period = period * unit;
    event->callback = callback;
    event->arg = arg;

    add_waiting_event(event);
}

void irq_timer_event(void){
    timer_event_t *event = NULL;
    uint64_t current_time;

    while(!list_is_empty(&waiting_event_q)){
        event = container_of(waiting_event_q.next, timer_event_t, head);
        current_time = timer_get_current_time(MICROSECOND);
        // The expired time has not arrived.
        if(event->expired_time > current_time){
            timer_set_countdown(event->expired_time - current_time, MICROSECOND);
            core_timer_enable();
            break;
        }

        list_remove(&event->head);
        event->callback(event->arg);

        if(event->period){
            event->registration_time = current_time;
            event->expired_time = current_time + event->period;
            add_waiting_event(event);
        }else{
            free(event);
        }
    }
}

void add_waiting_event(timer_event_t *event){
    if(event == NULL)   return;
    list_head_t *prev = &waiting_event_q;
    list_head_t *next = prev->next;

    disable_irq();
    while(!list_is_head_node(next, &waiting_event_q) && 
        container_of(next, timer_event_t, head)->expired_time <= event->expired_time){
        prev = next;
        next = next->next;
    }
    list_add(&event->head, prev, next);
    enable_irq();

    if(event == container_of(waiting_event_q.next, timer_event_t, head)){
        int countdown = event->expired_time - timer_get_current_time(MICROSECOND);
        timer_set_countdown(countdown, MICROSECOND);
        core_timer_enable();
    }
}
