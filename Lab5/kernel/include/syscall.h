#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#define SYS_NUM                 8

#define SYSCALL_GETPID          0
#define SYSCALL_UART_READ       1
#define SYSCALL_UART_WRITE      2
#define SYSCALL_EXEC            3
#define SYSCALL_FORK            4
#define SYSCALL_EXIT            5
#define SYSCALL_MAILBOX_CALL    6
#define SYSCALL_KILL            7

#ifndef __ASSEMBLER__
#include "types.h"

extern int syscall_getpid(void);
extern size_t syscall_uart_read(char buffer[], size_t size);
extern size_t syscall_uart_write(const char buffer[], size_t size);
extern int syscall_exec(const char *name, char *const argv[]);
extern int syscall_fork(void);
extern void syscall_exit(void);
extern int syscall_mailbox_call(uint8_t channel, uint32_t *mbox);
extern void syscall_kill(int pid);

#endif
#endif