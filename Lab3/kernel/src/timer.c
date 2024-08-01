#include "timer.h"
#include "peripheral.h"
#include "list.h"
#include "memory.h"
#include "mini_uart.h"
#include "string.h"

static LIST_HEAD(waiting_q);
static LIST_HEAD(blank_q);

typedef struct timer_event{
    list_head_t         anchor;
    uint64_t            event_time;
    uint64_t            expired_time;
    timer_event_cb_t    callback;
    char                msg[MSG_LEN_LIMIT];    
} timer_event_t;

timer_event_t* get_blank_event(void);
void timer_event_callback(const char *msg);
void add_waiting_event(timer_event_t *event);

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

void add_timeout_event(uint64_t duration, const char *msg){
    timer_event_t *event = get_blank_event();
    
    event->event_time = get_current_time();
    event->expired_time = event->event_time + duration;
    event->callback = timer_event_callback;
    if(msg){
        strncpy(msg, event->msg, MSG_LEN_LIMIT - 1);
        event->msg[MSG_LEN_LIMIT - 1] = '\0';
    }

    add_waiting_event(event);
}

void handler_irq_timer_event(void){
    timer_event_t *event = (timer_event_t*)waiting_q.next;
    event->callback(event->msg);
    list_remove((list_head_t*)event);
    list_add((list_head_t*)event, &blank_q, blank_q.next);

    if(!list_is_empty(&waiting_q)){
        event = (timer_event_t*)waiting_q.next;
        set_period(event->expired_time - get_current_time(), SECOND);
        core_timer_enable();
    }
}

timer_event_t* get_blank_event(){
    timer_event_t *event = NULL;
    if(list_is_empty(&blank_q)){
        event = (timer_event_t*)malloc(sizeof(timer_event_t));
    }else{
        event = (timer_event_t*)blank_q.next;
        list_remove(blank_q.next);
    }

    event->anchor.prev = event->anchor.next = NULL;
    event->event_time = 0;
    event->expired_time = 0;
    event->callback = NULL;
    event->msg[0] = '\0';

    return event;
}

void timer_event_callback(const char *msg){
    uart_putc('\r');
    if(str_len(msg)){
        uart_putln(msg);
    }else{
        uart_puts("<Timer>: The number of seconds since booting is ");
        uart_puts(uint_to_dec_str(get_current_time()));
        uart_putln(".");
    }
    uart_puts("$ ");
}


void add_waiting_event(timer_event_t *event){
    if(event == NULL){
        return;
    }
    
    list_head_t *prev = &waiting_q;
    list_head_t *next = prev->next;

    while(!list_node_is_head(next, &waiting_q) && ((timer_event_t*)next)->expired_time <= event->expired_time){
        prev = next;
        next = next->next;
    }

    list_add(&event->anchor, prev, next);

    if(event == (timer_event_t*)waiting_q.next){
        set_period(event->expired_time - get_current_time(), SECOND);
        core_timer_enable();
    }
}

