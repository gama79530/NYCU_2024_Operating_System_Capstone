#include "timer.h"
#include "peripheral.h"
#include "list.h"
#include "memory.h"
#include "exception.h"
#include "util.h"

static LIST_HEAD(waiting_event_q);

typedef struct timer_event{
    list_head_t             anchor;
    uint64_t                registration_time;
    uint64_t                expired_time;
    timer_event_callback_t  callback;
    void                    *arg;
} timer_event_t;

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

void timer_set_countdown(uint64_t countdown, enum tick unit){
    uint64_t tick;
    
    asm volatile("mrs   %0, cntfrq_el0": "=r"(tick));
    tick = tick * countdown / unit;
    asm volatile("msr    cntp_tval_el0, %0":: "r"(tick));
}

uint64_t timer_get_current_time(void){
    uint64_t physical_count, freq;
    asm volatile(
        "mrs   %0, cntpct_el0\n"
        "mrs   %1, cntfrq_el0\n"
        : "=r"(physical_count), "=r"(freq)
    );

    return physical_count / freq;
}

void timer_add_timeout_event(uint64_t countdown, timer_event_callback_t cb, void *arg){
    timer_event_t *event = NULL;
    if(countdown == 0)  return;

    event = (timer_event_t*)malloc(sizeof(timer_event_t));
    if(event == NULL)   return;

    event->registration_time = timer_get_current_time();
    event->expired_time = event->registration_time + countdown;
    event->callback = cb;
    event->arg = arg;

    list_head_t *prev = &waiting_event_q;
    list_head_t *next = prev->next;

    disable_interrupt_all();
    while(!list_is_head_node(next, &waiting_event_q) && 
          container_of(next, timer_event_t, anchor)->expired_time <= event->expired_time){
        prev = next;
        next = next->next;
    }
    list_add(&event->anchor, prev, next);
    enable_interrupt_all();

    if(event == container_of(waiting_event_q.next, timer_event_t, anchor)){
        timer_set_countdown(event->expired_time - timer_get_current_time(), SECOND);
        core_timer_enable();
    }
}

void handler_el1_5_timer_event(void){
    uint64_t current_time = timer_get_current_time();
    timer_event_t *event = NULL;

    while(!list_is_empty(&waiting_event_q)){
        event = container_of(waiting_event_q.next, timer_event_t, anchor);
        // The expired time has not arrived.
        if(event->expired_time > current_time){
            timer_set_countdown(event->expired_time - timer_get_current_time(), SECOND);
            core_timer_enable();
            break;
        }

        event->callback(event->arg);
        list_remove(&event->anchor);
        free(event);
    }
}