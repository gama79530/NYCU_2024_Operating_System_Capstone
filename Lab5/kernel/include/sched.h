#ifndef __SCHED_H__
#define __SCHED_H__

#define ALGORITHM_ROUND_ROBIN       0

#define CPU_CONTEXT_OFFSET          0

/* for register set */
#define S_FRAME_SIZE                (17 * 16)   // size of all saved registers 
#define S_X0                        0           // offset of x0 register in saved stack frame

#ifndef __ASSEMBLER__
#include "types.h"
#include "list.h"

typedef enum kernel_state{
    RUNNING,
    WAITING,
    ZOMBIE,
} kernel_state_t;

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

typedef struct task_struct{
    cpu_context_t   cpu_context;
    list_head_t     head;
    uint64_t        user_stack;
    uint64_t        flag;
    int             pid;
    kernel_state_t  state;
    bool            is_preemptive;
} task_struct_t;

typedef struct user_thread{
    uint64_t        regs[31];
    uint64_t        sp;
    uint64_t        pstate; // SPSR_EL1
    uint64_t        pc;     // ELR_EL1
} user_thread_t;

extern task_struct_t* get_current_task(void);
extern void set_current_task(task_struct_t *task);
extern void cpu_switch_to(task_struct_t *prev, task_struct_t *next);

int scheduling_init(void);
int get_current_pid(void);
int get_new_pid(void);

void enable_preemption(void);
void disable_preemption(void);
void idle(void);
void schedule();
void exit(void);
void wait(void);
void kill(int pid);

#endif
#endif