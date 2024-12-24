#include "sched.h"
#include "printf.h"
#include "config.h"
#include "util.h"
#include "thread.h"
#include "exception.h"
#include "frame.h"
#include "timer.h"

static int pid_seq = 1;
static task_struct_t init_task = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {&init_task.head, &init_task.head},
    0,
    FLAG_INIT,
    0,
    RUNNING,
    true
};

LIST_HEAD(running_task_q);
LIST_HEAD(waiting_task_s);
LIST_HEAD(terminated_task_q);

task_struct_t* get_next_task(void);
task_struct_t* round_robin(void);
void kill_zombies(void);

int scheduling_init(void){
    set_current_task(&init_task);
    timer_add_timeout_event(MICROSECOND, TIME_SHARING_MICROSEC, TIME_SHARING_MICROSEC, (timer_event_cb_t)schedule, NULL);
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

int get_current_pid(void){
    return get_current_task()->pid;
}

void schedule(){
    task_struct_t *current_task = get_current_task();
    if(!current_task->is_preemptive)    return;
    disable_preemption();
    task_struct_t *next_task = get_next_task();
    if(next_task != NULL){
        switch(current_task->state){
            case RUNNING:
                list_append(&current_task->head, &running_task_q);
                break;
            case WAITING:
                list_append(&current_task->head, &waiting_task_s);
                break;
            case ZOMBIE:
                list_append(&current_task->head, &terminated_task_q);
                break;
        }
        set_current_task(next_task);
        cpu_switch_to(current_task, next_task);
    }
    enable_preemption();
}

task_struct_t* round_robin(void){
    task_struct_t *next_task = NULL;
    if(!list_is_empty(&running_task_q)){
        next_task = container_of(running_task_q.next, task_struct_t, head);
        list_remove(&next_task->head);
    }else if(!list_is_empty(&waiting_task_s)){
        next_task = container_of(waiting_task_s.prev, task_struct_t, head);
        next_task->state = RUNNING;
        list_remove(&next_task->head);
    }

    return next_task;
}

void exit(void){
    disable_preemption();
    task_struct_t *current = get_current_task();
    current->state = ZOMBIE;
    enable_preemption();
    schedule();
}

task_struct_t* get_next_task(void){
    task_struct_t *next_task = NULL;
#if SCHEDULING_ALGORITHM == ALGORITHM_ROUND_ROBIN
    next_task = round_robin();
#else
    #error "Unsupported scheduling algorithm."
#endif
    return next_task;
}

void kill_zombies(void){
    task_struct_t *task = NULL;
    disable_irq();
#if KILL_ZOMBIES_ITER == 0
    while(!list_is_empty(&terminated_task_q)){
#else
    for(int i = 0; i < KILL_ZOMBIES_ITER && !list_is_empty(&terminated_task_q); i++){
#endif
        task = container_of(terminated_task_q.next, task_struct_t, head);
        list_remove(terminated_task_q.next);
        if(task->user_stack != 0){
            frame_free((void*)task->user_stack);
        }
        frame_free(task);
    }
    enable_irq();
}

int get_new_pid(void){
    return pid_seq++;
}

void idle(void){
    while(true){
        kill_zombies();
        schedule();
    }
}

void wait(void){
    disable_preemption();
    task_struct_t *current = get_current_task();
    current->state = WAITING;
    enable_preemption();
    schedule();
}