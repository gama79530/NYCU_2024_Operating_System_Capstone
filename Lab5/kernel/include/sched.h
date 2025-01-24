#ifndef __SCHED_H__
#define __SCHED_H__

#define CPU_CONTEXT_OFFSET              0

/* for register set */
#define STATE_FRAME_SIZE                (17 * 16)   // size of all saved registers 
#define STATE_FRAME_OFFSET_X0           0           // offset of x0 register in saved state stack frame

#ifndef __ASSEMBLER__
#include "types.h"
#include "list.h"

typedef enum task_state{
    RUNNING,
    WAITING,
    ZOMBIE,
} task_state_t;

typedef struct cpu_context{
    uint64_t        x19;
    uint64_t        x20;
    uint64_t        x21;
    uint64_t        x22;
    uint64_t        x23;
    uint64_t        x24;
    uint64_t        x25;
    uint64_t        x26;
    uint64_t        x27;
    uint64_t        x28;
    uint64_t        x29;   // Frame Pointer (FP)
    uint64_t        x30;   // link Register (LR)
    uint64_t        sp;    // stack pointer
} cpu_context_t;

/*
Note:  
    1. A thread with a smaller priority number is preferred but not guaranteed.  
       Also, a smaller priority number indicates a smaller portion of execution time in each epoch.  
    2. The "counter" field is used for managing the portion of execution time in each epoch.  
 
Restriction:  
    1. priority > 0  
*/
typedef struct kernel_task{
    cpu_context_t   cpu_context;
    list_head_t     anchor_sched;
    uint64_t        user_stack;
    uint64_t        flag;
    int             pid;
    task_state_t    state;
    uint8_t         priority;
    uint8_t         counter;
    bool            is_preemptive;
} kernel_task_t;

typedef struct user_thread{
    uint64_t        regs[31];
    uint64_t        sp;
    uint64_t        pstate; // SPSR_EL1
    uint64_t        pc;     // ELR_EL1
} user_thread_t;

extern list_head_t *running_task_queue;
extern list_head_t *waiting_task_queue;
extern list_head_t *terminated_task_queue;

extern kernel_task_t* get_current_task(void);
extern void set_current_task(kernel_task_t *task);
extern void cpu_switch_to(kernel_task_t *prev, kernel_task_t *next);

int scheduling_init(void);
void enable_preemption(void);
void disable_preemption(void);

kernel_task_t* find_task(int pid);
int get_current_pid(void);
int get_new_pid(void);

void idle(void);
void exit(void);
void wait(void);
void notify(int pid);
void kill(int pid);
void notify_task(kernel_task_t *task);
void kill_task(kernel_task_t *task);

/*
Note:
    1. reschedule = move current task to next epoch queue + context_switch
    2. reschedule is used for active invoking
*/
void reschedule(void);
void context_switch(void);


#endif
#endif