#include "sched.h"
#include "printf.h"
#include "config.h"
#include "util.h"
#include "thread.h"
#include "exception.h"
#include "frame.h"
#include "timer.h"

static int pid_seq = 1;
static kernel_task_t init_task = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {&init_task.anchor_sched, &init_task.anchor_sched},
    0,
    FLAG_INIT,
    0,
    RUNNING,
    PRIORITY_HIGH,
    PRIORITY_HIGH,
    true
};

static LIST_HEAD(running_task_q_1);
static LIST_HEAD(running_task_q_2);
static LIST_HEAD(waiting_task_q);
static LIST_HEAD(terminated_task_q);

list_head_t *running_task_queue = NULL;
list_head_t *next_epoch_queue = NULL;
list_head_t *waiting_task_queue = NULL;
list_head_t *terminated_task_queue = NULL;

static kernel_task_t* find_running_task(int pid);
static kernel_task_t* find_waiting_task(int pid);
static kernel_task_t* find_next_task(void);
static void kill_zombies(void);

int scheduling_init(void){
    running_task_queue = &running_task_q_1;
    next_epoch_queue = &running_task_q_2;
    waiting_task_queue = &waiting_task_q;
    terminated_task_queue = &terminated_task_q;
    set_current_task(&init_task);
    set_time_sharing_period(get_count_frequency() >> 5);
    enable_time_sharing();
#if VERBOSE != 0
    printf("***** scheduling_init success *****\n");
#endif
    return 0;
}

void enable_preemption(void){
    get_current_task()->is_preemptive = true;
}

void disable_preemption(void){
    get_current_task()->is_preemptive = false;
}

kernel_task_t* find_task(int pid){
    kernel_task_t *task = NULL;
    int pipe_len = 2;
    kernel_task_t* (*pipe[])(int) = {find_running_task, find_waiting_task};

    disable_preemption();
    for(int i = 0; i < pipe_len; i++){
        task = pipe[pipe_len](pid);
        if(task){
            break;
        }
    }
    enable_preemption();
    return task;
}

static kernel_task_t* find_running_task(int pid){
    kernel_task_t *task;
    list_head_t *node;

    // check current task
    task = get_current_task();
    if(task->pid == pid){
        return task;
    }

    int pipe_len = 2;
    list_head_t *pipe[] = {running_task_queue, next_epoch_queue};
    for(int i = 0; i < pipe_len; i++){
        list_head_t *queue = pipe[i];
        list_for_each(node, queue){
            task = container_of(node, kernel_task_t, anchor_sched);
            if(task->pid == pid){
                return task;
            }
        }
    }
    
    return NULL;
}

static kernel_task_t* find_waiting_task(int pid){
    kernel_task_t *task;
    list_head_t *node;

    // check waiting queue
    list_for_each(node, next_epoch_queue){
        task = container_of(node, kernel_task_t, anchor_sched);
        if(task->pid == pid){
            return task;
        }
    }
    
    return NULL;
}

int get_current_pid(void){
    return get_current_task()->pid;
}

int get_new_pid(void){
    int new_pid = pid_seq++;
    return new_pid;
}

void idle(void){
    while(true){
        disable_preemption();
        kill_zombies();
        enable_preemption();
        reschedule();
    }
}

static void kill_zombies(void){
    kernel_task_t *task = NULL;
#if KILL_ZOMBIES_ITER == 0
    while(!list_is_empty(terminated_task_queue))
#else
    for(int i = 0; i < KILL_ZOMBIES_ITER && !list_is_empty(terminated_task_queue); i++)
#endif
    {   task = container_of(terminated_task_queue->next, kernel_task_t, anchor_sched);
        list_remove(&task->anchor_sched);
        if(task->user_stack){
            frame_free((void*)task->user_stack);
        }
        frame_free(task);
    }
}

void exit(void){
    kernel_task_t *task = get_current_task();
    if(task->flag & (FLAG_INIT | FLAG_KERNEL_SHELL)){
        return;
    }
    disable_preemption();
    get_current_task()->state = ZOMBIE;
    enable_preemption();
    reschedule();
}

void wait(void){
    kernel_task_t *task = get_current_task();
    if(task->flag & FLAG_INIT){
        return;
    }
    disable_preemption();
    get_current_task()->state = WAITING;
    enable_preemption();
    reschedule();
}

void notify(int pid){
    disable_preemption();
    kernel_task_t *task = find_waiting_task(pid);
    if(task){
        task->state = RUNNING;
        list_remove(&task->anchor_sched);
        list_add_last(&task->anchor_sched, running_task_queue);
    }
    enable_preemption();
}

void kill(int pid){
    if(pid == get_current_pid()){
        exit();
    }else{
        kernel_task_t *task;
        disable_preemption();
        if(((task = find_running_task(pid)) && !(task->flag & (FLAG_INIT | FLAG_KERNEL_SHELL))) ||
           ((task = find_waiting_task(pid)) && !(task->flag & (FLAG_INIT | FLAG_KERNEL_SHELL)))){
            list_remove(&task->anchor_sched);
            task->state = ZOMBIE;
            list_add_last(&task->anchor_sched, terminated_task_queue);
        }
        enable_preemption();
        reschedule();
    }
}

void reschedule(void){
    get_current_task()->counter = 1;
    context_switch();
}

void context_switch(void){
    kernel_task_t *current_task = get_current_task();
    if(!current_task->is_preemptive){
        return;
    }
    
    disable_preemption();
    kernel_task_t *next_task = find_next_task();
    if(next_task){
        current_task->counter--;
        list_head_t *queue;
        switch(current_task->state){
            case RUNNING:
                if(current_task->counter){
                    queue = running_task_queue;
                }else{
                    queue = next_epoch_queue;
                    current_task->counter = current_task->priority;
                }
                break;
            case WAITING:
                queue = waiting_task_queue;
                break;
            case ZOMBIE:
                queue = terminated_task_queue;
                break;
        }
        list_add_last(&current_task->anchor_sched, queue);
        list_remove(&next_task->anchor_sched);
        set_current_task(next_task);
        cpu_switch_to(current_task, next_task);
    }
    enable_preemption();
}

static kernel_task_t* find_next_task(void){
    kernel_task_t *task = NULL;
    
    if(list_is_empty(running_task_queue) && !list_is_empty(next_epoch_queue)){
        swap(running_task_queue, next_epoch_queue);
    }

    if(!list_is_empty(running_task_queue)){
        task = container_of(running_task_queue->next, kernel_task_t, anchor_sched);
    }else if(!list_is_empty(waiting_task_queue)){
        task = container_of(waiting_task_queue->prev, kernel_task_t, anchor_sched);
    }

    return task;
}

void notify_task(kernel_task_t *task){
    disable_preemption();
    if(task){
        task->state = RUNNING;
        list_remove(&task->anchor_sched);
        list_add_last(&task->anchor_sched, running_task_queue);
    }
    enable_preemption();
}

void kill_task(kernel_task_t *task){
    if(task == get_current_task()){
        exit();
    }else if(task){
        disable_preemption();
        list_remove(&task->anchor_sched);
        task->state = ZOMBIE;
        list_add_last(&task->anchor_sched, terminated_task_queue);
        enable_preemption();
        reschedule();
    }
}
