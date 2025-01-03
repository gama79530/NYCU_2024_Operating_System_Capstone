#ifndef __PROCESS_H__
#define __PROCESS_H__

/*******************************************************************************************************
 * Note:
 *  1. FLAG_INIT is used for the initial task, and this task will never be paused or killed.
 *  2. FLAG_KERNEL_SHELL is used for the kernel shell, and this task will never be killed.
 *******************************************************************************************************/
#define FLAG_DUMMY              0
#define FLAG_INIT               (1 << 0)    
#define FLAG_KERNEL_SHELL       (1 << 1)
#define FLAG_ENTER_USER_MODE    (1 << 2)
#define FLAG_FORK               (1 << 3)

#ifndef __ASSEMBLER__
#include "types.h"

typedef void (*task_routine_t)(void *arg);

int create_task(uint64_t flag, uint8_t priority, task_routine_t routine, void *arg);
int enter_user_mode(uint64_t pc);

#endif
#endif
