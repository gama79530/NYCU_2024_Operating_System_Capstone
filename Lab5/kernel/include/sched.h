#ifndef __SCHED_H__
#define __SCHED_H__

#define CPU_CONTEXT_OFFSET      0

#ifndef __ASSEMBLER__
#include "types.h"
#include "list.h"

typedef struct cpu_context{
    uint64_t x19;
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t x29;   // Frame Pointer (FP)
    uint64_t x30;   // link Register (LR)
    uint64_t sp;    // stack pointer
} cpu_context_t;

typedef struct task_struct{
    cpu_context_t   cpu_context;
    list_head_t     anchor;
    uint64_t        pid;
    bool            is_preemptive;
} task_struct_t;

typedef void (*task_routine_t)(void *arg);

task_struct_t* get_current_task(void);

void preemption_enable(void);
void preemption_disable(void);

int thread_create(task_routine_t routine, void *arg);
void thread_exit(void);

void schedule(void);
#endif

#endif