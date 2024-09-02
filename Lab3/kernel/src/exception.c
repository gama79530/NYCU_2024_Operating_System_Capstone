#include "exception.h"
#include "mini_uart.h"
#include "util.h"
#include "peripheral.h"
#include "timer.h"
#include "string.h"
#include "memory.h"

static LIST_HEAD(waiting_q);
static LIST_HEAD(blank_q);

enum status{
    BLANK,
    WAITING,
    EXECUTING,
    DONE
};

typedef struct irq_task{
    list_head_t     anchor;
    enum status     status;
    uint8_t         priority;   // smaller number is more preemptive
    task_callback   handler;
} irq_task_t;

irq_task_t* get_blank_task(void);
void set_blank_task(irq_task_t* task, uint8_t priority, task_callback handler);
void add_waiting_task(irq_task_t *task);
void waiting_task_exec(void);

irq_task_t* get_blank_task(void){
    irq_task_t *task = NULL;
    if(list_is_empty(&blank_q)){
        task = (irq_task_t*)malloc(sizeof(irq_task_t));
    }else{
        task = (irq_task_t*)blank_q.next;
        list_remove(blank_q.next);
    }
    task->anchor.prev = task->anchor.next = NULL;
    task->status = BLANK;
    task->priority = 255;
    task->handler = NULL;
    
    return task;
}

void set_blank_task(irq_task_t* task, uint8_t priority, task_callback handler){
    task->priority = priority;
    task->handler = handler;
}

void add_waiting_task(irq_task_t *task){
    if(task == NULL){
        return;
    }
    
    list_head_t *prev = &waiting_q;
    list_head_t *next = prev->next;

    while(!list_node_is_head(next, &waiting_q) && ((irq_task_t*)next)->priority <= task->priority){
        prev = next;
        next = next->next;
    }

    list_add(&task->anchor, prev, next);
}

void waiting_task_exec(void){
    irq_task_t *task = NULL;
    while(!list_is_empty(&waiting_q) && (task = (irq_task_t*)waiting_q.next)->status == WAITING){
        enable_interrupt_all();
        task->status = EXECUTING;
        task->handler();
        task->status = DONE;
        disable_interrupt_all();
        list_remove((list_head_t*)task);
        list_add((list_head_t*)task, &blank_q, blank_q.next);
    }
}

void handler_irq_current_spelx_el1(void){
    irq_task_t *task = NULL;

    if(get32(IRQ_PENDING_1) & IRQ_AUX_INT){         // UART IRQ
        if(get32(AUX_MU_IIR) & AUX_IRQ_RX){         // Receiver holds valid byte
            interrupt_rx_clr();
            task = get_blank_task();
            task->status = WAITING;
            task->priority = 3;
            task->handler = handler_mini_uart_rx;
        }else if(get32(AUX_MU_IIR) & AUX_IRQ_TX){   // Transmit holding register empty
            interrupt_tx_clr();
            task = get_blank_task();
            task->status = WAITING;
            task->priority = 3;
            task->handler = handler_mini_uart_tx;
        }
    }else if(get32(CORE0_IRQ_SOURCE) & CNTPNSIRQ){  // timer IRQ
        core_timer_disable();
        task = get_blank_task();
        task->status = WAITING;
        task->priority = 1;
        task->handler = handler_irq_timer_event;
    }else{
        uart_putln("handler_irq_current_sp_elx");
    }

    if(task){
        add_waiting_task(task);
        waiting_task_exec();
    }
}

void handler_sync_lower_aarch64_el1(void){
    uint32_t esr, ec;

    asm volatile("mrs %0, esr_el1": "=r"(esr));
    ec = esr >> 26;
    switch(ec){
        case 0x15:  // SVC instruction execution
            uart_putln("User program synchronous exception generated by the SVC instruction.");
            break;
        default:
            uart_puts("esr = ");
            uart_putln(uint_to_dec_str(esr));
            uart_putln("handler_sync_lower_aarch64");
            break;
    }
}