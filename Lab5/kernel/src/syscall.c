#include "syscall.h"
#include "sched.h"
#include "config.h"
#include "mini_uart.h"
#include "cpio.h"
#include "string.h"
#include "memory.h"
#include "util.h"
#include "thread.h"
#include "mailbox.h"

int sys_getpid(void);
size_t sys_uart_read(char buffer[], size_t size);
size_t sys_uart_write(const char buffer[], size_t size);
int sys_exec(const char *name, char *const argv[]);
int sys_fork(void);
void sys_exit(void);
int sys_mailbox_call(uint8_t channel, uint32_t *mailbox);
void sys_kill(int pid);

void * const sys_table[] = {
    [SYSCALL_GETPID]        = sys_getpid,
    [SYSCALL_UART_READ]     = sys_uart_read,
    [SYSCALL_UART_WRITE]    = sys_uart_write,
    [SYSCALL_EXEC]          = sys_exec,
    [SYSCALL_FORK]          = sys_fork,
    [SYSCALL_EXIT]          = sys_exit,
    [SYSCALL_MAILBOX_CALL]  = sys_mailbox_call,
    [SYSCALL_KILL]          = sys_kill,
};

int sys_getpid(void){
    return get_current_pid();
}

size_t sys_uart_read(char buffer[], size_t size){
    size_t ret;
    for(ret = 0; ret < size; ret++){
        buffer[ret] = uart_getc();
    }
    return ret;
}

size_t sys_uart_write(const char buffer[], size_t size){
    size_t ret;
    for(ret = 0; ret < size; ret++){
        if(buffer[ret] == '\n'){
            uart_putc('\r');
        }
        uart_putc(buffer[ret]);
    }
    return ret;
}

int sys_exec(const char *name, char *const argv[]){
    void *current = get_cpio_base();
    file_info_t info;
    while(current != NULL){
        if(cpio_file_iter(&current, &info)){    // abnormal iter
            return -1;
        }else if(!strcmp(name, info.name)){      // file name is match
            // load program and execute
            void *user_prog = malloc(info.content_size);
            memcpy(user_prog, info.content, info.content_size);
            enter_user_mode((uint64_t)user_prog);
            return 0;
        }
    }

    return -1;
}

int sys_fork(void){
    return create_task(FLAG_FORK, PRIORITY_COPY, NULL, NULL);
}

void sys_exit(void){
    exit();
}

int sys_mailbox_call(uint8_t channel, uint32_t *mailbox){
    return mailbox_call(channel, mailbox);
}

void sys_kill(int pid){
    kill(pid);
}