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
#include "exception.h"
#include "signal.h"

typedef void (*thread_entry_t)(void);

/* Register the boot thread and initialize idle. Requires the dynamic allocator. */
bool thread_init(void);

/** Register the periodic scheduling event after timer/thread initialization. */
bool thread_start_preemption(void);

/** Honor a pending scheduling tick at outer IRQ exit, with IRQ masked.
 * All IRQ callbacks/shared dispatch state must be unwound before calling. */
void thread_preempt(void);

/* Create a runnable kernel thread; return its ID, or -1 on failure.
 * IDs may be reused after exit, before idle reclaims the old thread's memory. */
int thread_create(thread_entry_t entry);

/** Create a new runnable EL0 thread; return its ID, or -1 on failure.
 * Owns separate kernel/user stacks; user exit terminates the entire thread.
 * No image copy; entry must remain executable for the thread and all children. */
int thread_create_user(thread_entry_t entry);

/** Load a flat, position-independent initramfs image and publish a new EL0 thread.
 * Entry is the first instruction; all storage (including BSS) must be in the file. */
int thread_create_program(const char *name);

/** Replace the current user's image/stack context, preserving its PID/kernel stack.
 * Rewrite the active syscall frame on success; return 0, or -1 without replacement. */
int thread_exec(const char *name, exception_frame_t *frame);

/** Terminate a user process by ID; boot/idle/kernel threads are protected.
 * Return false for an invalid ID; successful self-termination does not return. */
bool thread_kill(int id);

/** Return whether an ID currently belongs to a live thread; zombies are excluded.
 * This does not retain the thread or prevent subsequent ID reuse. */
bool thread_is_alive(int id);

/** Terminate all live user threads from kernel thread context.
 * Boot/idle/kernel threads are preserved; idle reclaims the resulting zombies. */
void thread_kill_all_users(void);

/** Register a handler in the current user's executable image; NULL selects default. */
int thread_signal_register(int signal, signal_handler_t handler);

/** Queue a signal; unhandled SIGKILL terminates immediately, even during a syscall. */
int thread_signal_send(int id, int signal);

/** Prepare one pending handler on return to EL0; requires IRQ masked.
 * Called by the common exception restore path, including initial user entry. */
void thread_signal_deliver(exception_frame_t *frame);

/** Restore the kernel-saved user context and free the handler stack; return 0/-1. */
int thread_signal_return(exception_frame_t *frame);

/** Clone the current user's active stack and syscall frame; return child ID/-1.
 * Supports the lab's stack-only fork model, not general pointer relocation. */
int thread_fork(exception_frame_t *frame);

/** Return whether the current thread owns an EL0 user context. */
bool thread_is_user(void);

/** Validate a byte range within the current user's stack or loaded image.
 * This is syscall argument validation, not memory isolation without an MMU. */
bool thread_user_buffer_valid(uintptr_t address, size_t size);

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
