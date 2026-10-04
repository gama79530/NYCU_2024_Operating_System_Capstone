#ifndef LAB5_C_SYSCALL_H
#define LAB5_C_SYSCALL_H

#include "exception.h"

/* EL0 wrappers: enter the kernel through svc #0; do not call from EL1. */

/** Return the current user thread's ID. */
int sys_getpid(void);
/** Read bytes into a user-stack buffer; return the byte count or -1. */
long sys_uart_read(char *buffer, size_t size);
/** Write a user-stack buffer; return the byte count or -1. */
long sys_uart_write(const char *buffer, size_t size);
/** Replace the current program; currently unsupported and returns -1. */
int sys_exec(const char *name, char *const argv[]);
/** Fork the current user thread; return child ID, 0 in the child, or -1. */
int sys_fork(void);
/** End the current user thread; this call does not return. */
_Noreturn void sys_exit(int status);
/** Call the mailbox channel; currently unsupported and returns -1. */
int sys_mbox_call(unsigned char channel, unsigned int *mbox);
/** Terminate the specified process; currently unsupported and has no effect. */
void sys_kill(int pid);

/* EL1 exception handler entry. */

/** Dispatch an EL0 svc 0 with IRQ enabled and outside interrupt nesting state.
 * x8 selects the call, x0/x1 carry arguments, and x0 receives the result.
 * exit never returns; unsupported calls and invalid stack buffers return -1. */
void syscall_dispatch(exception_frame_t *frame);

#endif
