#include "timer.h"
#include "peripheral.h"
#include "list.h"
#include "memory.h"
#include "exception.h"
#include "util.h"
#include "sched.h"
#include "config.h"
#include "exception.h"

#define SAFTY_SHIFT     100

typedef struct timer_event{
    list_head_t             head;
    uint64_t                registration_count;
    uint64_t                expired_count;
    uint64_t                period_count;
    timer_event_cb_t        callback;
    void                    *arg;
} timer_event_t;

static LIST_HEAD(waiting_event_q);
timer_event_t time_sharing_event = {
    {&time_sharing_event.head, &time_sharing_event.head},
    0,
    0,
    TIME_SHARING_DEFAULT_PERIOD,
    (timer_event_cb_t)context_switch,
    NULL
};
bool time_sharing_flag = false;

void add_waiting_event(timer_event_t *event);

uint64_t get_count_frequency(void){
    uint64_t frequency = 0;
    asm volatile(
        "mrs    %0, cntfrq_el0"
        : "=r"(frequency)
    );
    return frequency;
}

uint64_t get_physical_count(void){
    uint64_t count = 0;
    asm volatile(
        "mrs    %0, cntpct_el0"
        : "=r"(count)
    );
    return count;
}

uint64_t time_to_count(time_unit_t unit, uint64_t time){
    return time * get_count_frequency() / unit ;
}

uint64_t count_to_time(time_unit_t unit, uint64_t count){
    return count * unit / get_count_frequency();
}

int timer_init(void){
    uint64_t setting;
    asm volatile(
        "mrs    %0, cntkctl_el1"
        : "=r"(setting)
    );
    setting |= 1;
    asm volatile(
        "msr   cntkctl_el1, %0"
        :
        : "r"(setting)
    );
    return 0;
}

void enable_core0_timer(void){
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

void disable_core0_timer(void){
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
    asm volatile(
        "mov    x0, 0\n"
        "msr    cntp_ctl_el0, x0\n"
        :
        :
        : "x0"
    );
}

void timer_set_countdown(uint64_t countdown){
    asm volatile(
        "msr    cntp_tval_el0, %0" : : 
        "r"(countdown)
    );
}

uint64_t timer_get_time(time_unit_t unit){
    return count_to_time(unit, get_physical_count());
}

void timer_add_timeout_event(
    time_unit_t unit,
    uint64_t time,
    uint64_t period,
    timer_event_cb_t callback,
    void *arg
){
    timer_add_countdown_event(
        time_to_count(unit, time),
        time_to_count(unit, period),
        callback,
        arg
    );
}

void timer_add_countdown_event(
    uint64_t countdown,
    uint64_t period_count,
    timer_event_cb_t callback,
    void *arg
){
    timer_event_t *event = NULL;
    if(countdown == 0)  return;
    
    event = (timer_event_t*)malloc(sizeof(timer_event_t));
    if(event == NULL)   return;

    event->registration_count = get_physical_count();
    event->expired_count = event->registration_count + countdown;
    event->period_count = period_count;
    event->callback = callback;
    event->arg = arg;

    add_waiting_event(event);
}

void add_waiting_event(timer_event_t *event){
    if(event == NULL)   return;
    list_head_t *cursor;

    disable_irq();
    for(cursor = waiting_event_q.prev;
        !list_is_head_node(cursor, &waiting_event_q) &&
        container_of(cursor, timer_event_t, head)->expired_count > event->expired_count;
        cursor = cursor->prev);
    list_add(&event->head, cursor, cursor->next);
    enable_irq();

    if(event == container_of(waiting_event_q.next, timer_event_t, head)){
        int count = get_physical_count();
        int countdown = event->expired_count > count + SAFTY_SHIFT ? event->expired_count - count : SAFTY_SHIFT;
        timer_set_countdown(countdown);
        enable_core0_timer();
    }
}

void irq_timer_event(void){
    timer_event_t *event = NULL;
    uint64_t count;

    while(!list_is_empty(&waiting_event_q)){
        event = container_of(waiting_event_q.next, timer_event_t, head);
        count = get_physical_count();
        
        // The expired time has not arrived.
        if(event->expired_count > count){
            int countdown = event->expired_count > count + SAFTY_SHIFT ? event->expired_count - count : SAFTY_SHIFT;
            timer_set_countdown(countdown);
            enable_core0_timer();
            break;
        }

        list_remove(&event->head);
        event->callback(event->arg);

        if(event->period_count){
            event->registration_count = count;
            event->expired_count = count + event->period_count;
            add_waiting_event(event);
        }else{
            free(event);
        }
    }
}

void enable_time_sharing(void){
    if(!time_sharing_flag){
        time_sharing_event.registration_count = get_physical_count();
        time_sharing_event.expired_count = time_sharing_event.registration_count + time_sharing_event.expired_count;
        add_waiting_event(&time_sharing_event);
        time_sharing_flag = true;
    }
}

void disable_time_sharing(void){
    if(time_sharing_flag){
        disable_irq();
        disable_core0_timer();
        list_remove(&time_sharing_event.head);
        if(!list_is_empty(&waiting_event_q)){
            timer_event_t *event = container_of(waiting_event_q.next, timer_event_t, head);
            int count = get_physical_count();
            int countdown = event->expired_count > count + SAFTY_SHIFT ? event->expired_count - count : SAFTY_SHIFT;
            timer_set_countdown(countdown);
            enable_core0_timer();
        }
        enable_irq();
        time_sharing_flag = false;
    }
}

bool get_time_sharing_flag(void){
    return time_sharing_flag;
}

void set_time_sharing_period(uint64_t period_count){
    if(period_count){
        time_sharing_event.period_count = period_count;
    }
}
