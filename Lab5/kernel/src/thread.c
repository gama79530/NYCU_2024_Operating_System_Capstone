#include "thread.h"
#include "sched.h"
#include "config.h"
#include "exception.h"
#include "frame.h"
#include "util.h"
#include "arm_v8.h"

extern list_head_t running_task_q;
extern list_head_t waiting_task_q;
extern list_head_t terminated_task_q;
extern void thread_start(void);

user_thread_t* get_user_thread(task_struct_t *task);

int thread_create(uint64_t flag, task_routine_t routine, void *arg){
    disable_preemption();
    task_struct_t *current_task = get_current_task();
    task_struct_t *new_task = (task_struct_t*)frame_alloc(KERNEL_THREAD_ORDER);
    if(new_task == NULL){
        return -1;
    }

    user_thread_t *user_thread = get_user_thread(new_task);
    memzero(user_thread, S_FRAME_SIZE);
    memzero((void*)&new_task->cpu_context, sizeof(cpu_context_t));
    new_task->head.prev = new_task->head.next = &new_task->head;

    if(routine == NULL){
        new_task->cpu_context.x19 = (uint64_t)routine;
        new_task->cpu_context.x20 = (uint64_t)arg;
        new_task->cpu_context.x21 = (flag & FLAG_ENTER_USER_MODE);
        new_task->user_stack = 0;
    }else{
        new_task->user_stack = (uint64_t)frame_alloc(USER_THREAD_STACK_ORDER);  // allocate new user stack
        if(new_task->user_stack == 0){
            frame_free(new_task);
            return -1;
        }
        user_thread_t *current_thread = get_user_thread(current_task);
        *user_thread = *current_thread;
        user_thread->regs[0] = 0;
        user_thread->sp = new_task->user_stack + USER_THREAD_STACK_SIZE;
        if(flag & FLAG_FORK){
            uint64_t offset = current_task->user_stack + USER_THREAD_STACK_SIZE - current_thread->sp;
            user_thread->sp -= offset;
            memcpy((void*)user_thread->sp, (void*)current_thread->sp, offset);
        }
    }

    new_task->flag = flag;
    new_task->pid = get_new_pid();
    new_task->state = RUNNING;
    new_task->is_preemptive = false;
    new_task->cpu_context.x30 = (uint64_t)thread_start;
	new_task->cpu_context.sp = (uint64_t)user_thread;
	list_append(&new_task->head, &running_task_q);
    enable_preemption();

	return new_task->pid;
}

user_thread_t* get_user_thread(task_struct_t *task){
	uint64_t user_thread = (uint64_t)task + KERNEL_THREAD_SIZE - S_FRAME_SIZE;
	return (user_thread_t*)user_thread;
}

int enter_user_mode(uint64_t pc){
    task_struct_t *current_task = get_current_task();
    current_task->user_stack = (uint64_t)frame_alloc(USER_THREAD_STACK_ORDER);  // allocate new user stack
    if(current_task->user_stack == 0){
        return -1;
    }
    user_thread_t *current_thread = get_user_thread(current_task);
    memzero(current_thread, S_FRAME_SIZE);
    current_thread->sp = current_task->user_stack + USER_THREAD_STACK_SIZE;
    current_thread->pstate = SPSR_MODE_EL0t;
    current_thread->pc = pc;

    return 0;
}
