#include "task_queue.h"

#include "allocator.h"
#include "config.h"
#include "daif.h"
#include "list.h"
#include "util.h"

/* Private types */

typedef struct task_node {
    list_head_t anchor;
    task_priority_t priority;
    task_callback_t callback;
    void *data;
} task_node_t;

/* Private function declarations */

/* Allocate a task node from the local cache or kernel allocator. */
static task_node_t *task_alloc(void);

/* Reset a task node and cache or release it. */
static void task_free(task_node_t *task);

/* Private data */

static LIST_HEAD(pending_queue);
static LIST_HEAD(free_queue);
static size_t active_count;
static size_t cached_count;
static bool task_running;
static task_priority_t current_task_priority;

/* Function implementations */

static task_node_t *task_alloc(void)
{
    task_node_t *task;

    if (active_count >= CONFIG_TASK_QUEUE_MAX_TASKS) {
        return NULL;
    }

    if (!list_is_empty(&free_queue)) {
        task = container_of(free_queue.next, task_node_t, anchor);
        list_remove(&task->anchor);
        cached_count--;
    } else {
        task = malloc(sizeof(*task));
        if (task == NULL) {
            return NULL;
        }
        LIST_INIT(&task->anchor);
    }

    active_count++;
    return task;
}

static void task_free(task_node_t *task)
{
    if (task == NULL) {
        return;
    }

    task->callback = NULL;
    task->data = NULL;
    active_count--;

    if (!kernel_allocator_is_ready() || cached_count < CONFIG_TASK_QUEUE_CACHE_SIZE) {
        list_add_last(&task->anchor, &free_queue);
        cached_count++;
    } else {
        free(task);
    }
}

bool task_queue_push(task_priority_t priority, task_callback_t callback, void *data)
{
    task_node_t *task;
    list_head_t *cursor;

    if (callback == NULL) {
        return false;
    }

    task = task_alloc();
    if (task == NULL) {
        return false;
    }

    task->priority = priority;
    task->callback = callback;
    task->data = data;

    /* Lower numeric priority runs first; insert after existing equal-priority tasks. */
    list_for_each(cursor, &pending_queue) {
        task_node_t *queued_task = container_of(cursor, task_node_t, anchor);
        if (queued_task->priority > priority) {
            list_add(&task->anchor, cursor->prev, cursor);
            return true;
        }
    }

    list_add_last(&task->anchor, &pending_queue);
    return true;
}

void task_queue_run(void)
{
    while (!list_is_empty(&pending_queue)) {
        task_node_t *task = container_of(pending_queue.next, task_node_t, anchor);
        task_callback_t callback = task->callback;
        void *data = task->data;
        task_priority_t priority = task->priority;
        bool previous_task_running;
        task_priority_t previous_priority;

        /* A nested IRQ may only preempt the active task with a higher-priority task. */
        if (task_running && priority >= current_task_priority) {
            return;
        }

        list_remove(&task->anchor);
        task_free(task);

        previous_task_running = task_running;
        previous_priority = current_task_priority;
        task_running = true;
        current_task_priority = priority;

        daif_irq_enable();
        callback(data);
        daif_irq_disable();

        task_running = previous_task_running;
        current_task_priority = previous_priority;
    }
}
