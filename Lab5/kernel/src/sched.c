#include "sched.h"
#include "frame.h"
#include "config.h"
#include "util.h"

extern void thread_start(void);
extern void context_switch(task_struct_t *prev_task, task_struct_t *next_wask);
extern void context_load(task_struct_t *task);

static LIST_HEAD(task_waiting_q);
static task_struct_t init_task = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {NULL, NULL}, 0, true};
static task_struct_t *current_task = &init_task;

task_struct_t* round_robin(void);

void preemption_enable(void){
    current_task->is_preemptive = true;
}

void preemption_disable(void){
    current_task->is_preemptive = false;
}

int thread_create(task_routine_t routine, void *arg){
    preemption_disable();
    task_struct_t *task = (task_struct_t*)frame_alloc(0);
    if(task == NULL)    return -1;

    task->pid = address_to_frame_idx((void*)task);
    task->is_preemptive = false;

    task->cpu_context.x19 = (uint64_t)routine;
    task->cpu_context.x20 = (uint64_t)arg;
    task->cpu_context.x30 = (uint64_t)thread_start;
    task->cpu_context.sp = (uint64_t)task + FRAME_SIZE;

    list_append(&task->anchor, &task_waiting_q);

    preemption_enable();
    return 0;
}

void thread_exit(void){
    preemption_disable();
    // deallocate current_task
    frame_free((void*)current_task);

    // select next task, round robin
    task_struct_t *task = round_robin();
    if(task == NULL)    task = &init_task;

    // config current task
    current_task = task;

    // context load
    context_load(current_task);
}

void schedule(void){
    if(!current_task->is_preemptive)  return;

    preemption_disable();
    task_struct_t *prev_task = current_task;
    task_struct_t *next_task = round_robin();

    if(next_task != NULL){
        // config current task and put prev_task into waiting queue
        current_task = next_task;
        list_append(&prev_task->anchor, &task_waiting_q);

        // context switch
        context_switch(prev_task, next_task);
    }

    preemption_enable();
}

task_struct_t* round_robin(void){
    if(list_is_empty(&task_waiting_q))  return NULL;
    task_struct_t *task = container_of(task_waiting_q.next, task_struct_t, anchor);
    list_remove(&task->anchor);
    task->anchor.prev = task->anchor.next = NULL;
    return task;
}

task_struct_t* get_current_task(void){
    return current_task;
}
