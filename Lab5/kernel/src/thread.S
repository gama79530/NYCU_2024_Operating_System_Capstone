#include "macro.S"
#include "sched.h"
#include "thread.h"

.global enter_kernel_task
enter_kernel_task:
    bl      enable_preemption
    str     x21, [sp, #-8]!
    cbz     x19, to_user_mode   // not a kernel thread
    mov     x0, x20
    blr     x19
to_user_mode:
    ldr     x21, [sp], #8
    cmp     x21, #FLAG_ENTER_USER_MODE
    bne     terminated_task
    bl      disable_irq
    load_state 0

terminated_task:
    bl      exit
    ret