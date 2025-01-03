#include "thread.h"
#include "sched.h"
#include "config.h"
#include "exception.h"
#include "frame.h"
#include "util.h"
#include "arm_v8.h"

extern void enter_kernel_task(void);

user_thread_t* find_thread(kernel_task_t *task);

int create_task(uint64_t flag, uint8_t priority, task_routine_t routine, void *arg){
    disable_preemption();
    kernel_task_t *current_task = get_current_task();
    kernel_task_t *new_task = (kernel_task_t*)frame_alloc(TASK_STACK_ORDER);
    if(new_task == NULL){
        return -1;
    }
    user_thread_t *new_thread = find_thread(new_task);
    memzero(new_thread, STATE_FRAME_SIZE);
    memzero((void*)&new_task->cpu_context, sizeof(cpu_context_t));

    if(routine != NULL){
        new_task->cpu_context.x19 = (uint64_t)routine;
        new_task->cpu_context.x20 = (uint64_t)arg;
        new_task->user_stack = 0;
    }else{
        new_task->user_stack = (uint64_t)frame_alloc(THREAD_STACK_ORDER);  // allocate new user stack
        if(new_task->user_stack == 0){
            frame_free(new_task);
            return -1;
        }
        user_thread_t *current_thread = find_thread(current_task);
        *new_thread = *current_thread;
        new_thread->regs[0] = 0;
        new_thread->sp = new_task->user_stack + THREAD_STACK_SIZE;
        if(flag & FLAG_FORK){
            uint64_t offset = current_task->user_stack + THREAD_STACK_SIZE - current_thread->sp;
            new_thread->sp -= offset;
            memcpy((void*)new_thread->sp, (void*)current_thread->sp, offset);
        }
    }

    new_task->cpu_context.x21 = (flag & FLAG_ENTER_USER_MODE);
    new_task->cpu_context.x30 = (uint64_t)enter_kernel_task;
    new_task->cpu_context.sp = (uint64_t)new_thread;
    LIST_INIT(&new_task->anchor_sched);
    new_task->flag = flag;
    new_task->pid = get_new_pid();
    new_task->state = RUNNING;
    new_task->priority = (priority ? priority : current_task->priority);
    new_task->counter = new_task->priority;
    new_task->is_preemptive = false;
    list_add_last(&new_task->anchor_sched, running_task_queue);
    enable_preemption();

    return new_task->pid;
}

user_thread_t* find_thread(kernel_task_t *task){
	uint64_t user_thread = (uint64_t)task + TASK_STACK_SIZE - STATE_FRAME_SIZE;
	return (user_thread_t*)user_thread;
}

int enter_user_mode(uint64_t pc){
    kernel_task_t *current_task = get_current_task();
    current_task->user_stack = (uint64_t)frame_alloc(THREAD_STACK_ORDER);  // allocate new user stack
    if(current_task->user_stack == 0){
        return -1;
    }
    user_thread_t *current_thread = find_thread(current_task);
    memzero(current_thread, STATE_FRAME_SIZE);
    current_thread->sp = current_task->user_stack + THREAD_STACK_SIZE;
    current_thread->pstate = SPSR_MODE_EL0t;
    current_thread->pc = pc;

    return 0;
}
