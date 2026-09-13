#ifndef LAB5_C_TASK_QUEUE_H
#define LAB5_C_TASK_QUEUE_H

#include "types.h"

typedef void (*task_callback_t)(void *data);

typedef enum {
    TASK_PRIORITY_TIMER = 0,
    TASK_PRIORITY_UART_RX = 1,
    TASK_PRIORITY_UART_TX = 2,
} task_priority_t;

/*
 * Enqueue deferred work in priority order.
 *
 * Lower numeric values run first. Tasks with the same priority keep FIFO order.
 */
bool task_queue_push(task_priority_t priority, task_callback_t callback, void *data);

/*
 * Run queued tasks with IRQ enabled.
 *
 * A nested invocation only runs tasks whose priority is higher than the task
 * it interrupted. The outer invocation resumes the remaining queue afterward.
 */
void task_queue_run(void);

#endif
