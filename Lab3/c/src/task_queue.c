#include "task_queue.h"

#include "allocator.h"
#include "config.h"
#include "exception.h"
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

/* Allocate a task node from the free list or the simple heap. */
static task_node_t *task_alloc(void);

/* Reset a task node and return it to the free list. */
static void task_free(task_node_t *task);

/* Private data */

static LIST_HEAD(pending_queue);
static LIST_HEAD(free_queue);
static size_t allocated_count;

/* Function implementations */

static task_node_t *task_alloc(void)
{
    task_node_t *task;

    if (!list_is_empty(&free_queue)) {
        task = container_of(free_queue.next, task_node_t, anchor);
        list_remove(&task->anchor);
        return task;
    }

    if (allocated_count >= CONFIG_TASK_QUEUE_MAX_TASKS) {
        return NULL;
    }

    task = simple_malloc(sizeof(*task));
    if (task == NULL) {
        return NULL;
    }

    allocated_count++;
    LIST_INIT(&task->anchor);
    return task;
}

static void task_free(task_node_t *task)
{
    if (task == NULL) {
        return;
    }

    task->callback = NULL;
    task->data = NULL;
    list_add_last(&task->anchor, &free_queue);
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

        list_remove(&task->anchor);
        task_free(task);

        callback(data);
    }
}
