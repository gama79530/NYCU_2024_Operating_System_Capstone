#include "syscall.h"

.global syscall_getpid
syscall_getpid:
    mov     w8, #SYSCALL_GETPID
    svc     #0
    ret

.global syscall_uart_read
syscall_uart_read:
    mov     w8, #SYSCALL_UART_READ
    svc     #0
    ret

.global syscall_uart_write
syscall_uart_write:
    mov     w8, #SYSCALL_UART_WRITE
    svc     #0
    ret

.global syscall_exec
syscall_exec:
    mov     w8, #SYSCALL_EXEC
    svc     #0
    ret

.global syscall_fork
syscall_fork:
    mov     w8, #SYSCALL_FORK
    svc     #0
    ret

.global syscall_exit
syscall_exit:
    mov     w8, #SYSCALL_EXIT
    svc     #0
    ret

.global syscall_mailbox_call
syscall_mailbox_call:
    mov     w8, #SYSCALL_MAILBOX_CALL
    svc     #0
    ret

.global syscall_kill
syscall_kill:
    mov     w8, #SYSCALL_KILL
    svc     #0
    ret
