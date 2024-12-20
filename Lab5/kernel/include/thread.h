#ifndef __PROCESS_H__
#define __PROCESS_H__
#include "types.h"

#define FLAG_DUMMY              0
#define FLAG_INIT               (1 << 0)
#define FLAG_ENTER_USER_MODE    (1 << 1)
#define FLAG_FORK               (1 << 2)


typedef void (*task_routine_t)(void *arg);

int thread_create(uint64_t flag, task_routine_t routine, void *arg);
int enter_user_mode(uint64_t pc);

#endif
