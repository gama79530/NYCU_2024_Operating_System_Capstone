#ifndef LAB5_C_THREAD_H
#define LAB5_C_THREAD_H

/* Shared with switch.S; cpu_context_t is the first member of thread metadata. */
#define THREAD_CONTEXT_X19_X20 0
#define THREAD_CONTEXT_X21_X22 16
#define THREAD_CONTEXT_X23_X24 32
#define THREAD_CONTEXT_X25_X26 48
#define THREAD_CONTEXT_X27_X28 64
#define THREAD_CONTEXT_FP_LR 80
#define THREAD_CONTEXT_SP 96
#define THREAD_CONTEXT_SIZE 104

#ifndef __ASSEMBLER__

#include "types.h"

typedef void (*thread_entry_t)(void);

/* Register the boot thread and initialize idle. Requires the dynamic allocator. */
bool thread_init(void);

/* Create a runnable kernel thread; return its ID, or -1 on failure.
 * IDs may be reused after exit, before idle reclaims the old thread's memory. */
int thread_create(thread_entry_t entry);

/* Return the current thread ID (boot: 0, idle: 1), or -1 before initialization. */
int thread_current_id(void);

/* Count created threads, including zombies awaiting idle reclamation. */
size_t thread_count(void);

/* Yield in kernel thread context only, never in IRQ/deferred callbacks or EL0. */
void schedule(void);

/* End a created thread. Boot and idle must not call this API. */
_Noreturn void thread_exit(void);

#endif

#endif
