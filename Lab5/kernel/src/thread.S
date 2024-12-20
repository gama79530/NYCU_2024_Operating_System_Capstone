#include "macro.S"
#include "sched.h"

.global thread_start
thread_start:
    bl      enable_preemption
    cbz     x19, to_user_mode   // not a kernel thread
    mov     x0, x20
    blr     x19

.global to_user_mode
to_user_mode:
    bl      disable_irq
    load_state 0

