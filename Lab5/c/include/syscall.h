#ifndef LAB5_C_SYSCALL_H
#define LAB5_C_SYSCALL_H

#include "exception.h"
#include "signal.h"

/* EL0 wrappers: enter the kernel through svc #0; do not call from EL1. */

/** Return the current user thread's ID. */
int sys_getpid(void);
/** Read bytes into a user stack/image buffer; return the byte count or -1. */
long sys_uart_read(char *buffer, size_t size);
/** Write a user stack/image buffer; return the byte count or -1. */
long sys_uart_write(const char *buffer, size_t size);
/** Replace the current program from initramfs; return -1 on failure only.
 * argv is reserved and ignored in Lab5; the new program starts at image byte 0. */
int sys_exec(const char *name, char *const argv[]);
/** Fork the current user thread; return child ID, 0 in the child, or -1. */
int sys_fork(void);
/** End the current user thread; this call does not return. */
_Noreturn void sys_exit(int status);
/** Call property channel 8 with a 16-byte-aligned user stack/image buffer.
 * Return 1 on success, 0 on mailbox failure, or -1 for an invalid user range/channel. */
int sys_mbox_call(unsigned char channel, unsigned int *mbox);
/** Terminate a user process; an invalid/protected ID has no effect. */
void sys_kill(int pid);
/** Register a user handler, or NULL for the default action; return 0 or -1. */
int sys_sigregister(int signal, signal_handler_t handler);
/** Send a signal to a user process; return 0 or -1 for an invalid target/signal. */
int sys_sigsend(int pid, int signal);
/** Restore the interrupted context; return -1 only if no signal handler is active. */
int sys_sigreturn(void);

/* EL1 exception handler entry. */

/** Dispatch an EL0 svc 0 with IRQ enabled and outside interrupt nesting state.
 * x8 selects the call, x0/x1 carry arguments, and x0 receives the result.
 * exit never returns; successful exec enters the replacement program.
 * Unsupported calls and invalid user buffers set frame->regs[0] to -1,
 * which is restored to x0 as the user-visible syscall result. */
void syscall_dispatch(exception_frame_t *frame);

#endif
