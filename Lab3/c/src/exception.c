#include "exception.h"
#include "common.h"
#include "list.h"
#include "memory.h"
#include "mini_uart.h"
#include "peripheral.h"


typedef struct irq_task {
    list_head_t anchor;
    uint8_t priority;  // lower value means higher priority
    irq_cb callback;
} irq_task_t;

// from current EL while using SP_ELx
void handler_type05(void);  // irq

// from lower EL and at least one lower EL is AArch64
void handler_type08(void);  // synchronous

irq_task_t *get_blank_task(void);
void set_task(irq_task_t *task, uint8_t priority, irq_cb callback);
void add_task_to_waiting_q(irq_task_t *task);
void process_waiting_tasks(void);

static LIST_HEAD(waiting_q);
static LIST_HEAD(blank_q);


void handler_type05(void)
{
    irq_task_t *task = NULL;

    if (get32(CORE0_IRQ_SOURCE) & CNTPNSIRQ) {
    }
}


void handler_type08(void)
{
    uint32_t esr, ec;

    asm volatile("mrs %0, esr_el1" : "=r"(esr));
    ec = esr >> 26;

    switch (ec) {
    case 0x15:  // SVC instruction execution
        mini_uart_putln("Trapped SVC instruction executed in AArch64 state.");
        break;
    default:
        mini_uart_puts("Unknown exception type8: esr_el1 = ");
        mini_uart_putln(uint64_to_hex_str(esr, 8, NULL));
        break;
    }
}


irq_task_t *get_blank_task(void)
{
    irq_task_t *task = NULL;
    if (list_is_empty(&blank_q)) {
        task = (irq_task_t *) startup_alloc(sizeof(irq_task_t));
    } else {
        task = (irq_task_t *) blank_q.next;
        list_remove(blank_q.next);
    }

    task->anchor.prev = task->anchor.next = NULL;
    task->priority = 255;
    task->callback = NULL;

    return task;
}


void set_task(irq_task_t *task, uint8_t priority, irq_cb callback)
{
    task->priority = priority;
    task->callback = callback;
}


void add_task_to_waiting_q(irq_task_t *task)
{
    if (task == NULL || task->callback == NULL) {
        return;
    }

    list_head_t *prev = waiting_q.prev;
    list_head_t *next = &waiting_q;
    irq_task_t *prev_task;

    while (true) {
        if (list_node_is_head(prev, &waiting_q)) {
            break;
        }
        prev_task = (irq_task_t *) container_of(prev, irq_task_t, anchor);
        if (prev_task->priority <= task->priority) {
            break;
        }
        next = prev;
        prev = prev->prev;
    }

    list_add(&task->anchor, prev, next);
}


void process_waiting_tasks(void)
{
    irq_task_t *task = NULL;
    while (!list_is_empty(&waiting_q)) {
        task = (irq_task_t *) container_of(waiting_q.next, irq_task_t, anchor);
        list_remove(&task->anchor);
        unmask_interrupt();
        task->callback();
        mask_interrupt();
        list_add_last(&task->anchor, &blank_q);
    }
}