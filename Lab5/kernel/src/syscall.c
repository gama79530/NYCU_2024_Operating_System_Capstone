#include "syscall.h"
// #include "config.h"
// #include "mini_uart.h"
// #include "sched.h"
// #include "cpio.h"
// #include "util.h"
// #include "memory.h"
// #include "string.h"

// int sys_getpid(void);
// size_t sys_uart_read(char buffer[], size_t size);
// size_t sys_uart_write(const char buffer[], size_t size);
// int sys_exec(const char *name, char *const argv[]);
// int sys_fork(void);
// void sys_exit(void);
// int sys_mailbox_call(uint8_t channel, uint32_t *mbox);
// void sys_kill(int pid);

void * const sys_table[] = {
    // [SYSCALL_GETPID]        = sys_getpid,
    // [SYSCALL_UART_READ]     = sys_uart_read,
    // [SYSCALL_UART_WRITE]    = sys_uart_write,
    // [SYSCALL_EXEC]          = sys_exec,
    // [SYSCALL_FORK]          = sys_fork,
    // [SYSCALL_EXIT]          = sys_exit,
    // [SYSCALL_MAILBOX_CALL]  = sys_mailbox_call,
    // [SYSCALL_KILL]          = sys_kill,
};

// int sys_getpid(void){
//     return get_pid();
// }

// size_t sys_uart_read(char buffer[], size_t size){
//     size_t ret;
//     for(ret = 0; ret < size; ret++){
//         buffer[ret] = uart_getc();
//     }
//     return ret;
// }

// size_t sys_uart_write(const char buffer[], size_t size){
//     size_t ret;
//     for(ret = 0; ret < size; ret++){
//         if(buffer[ret] == '\n'){
//             uart_putc('\r');
//         }
//         uart_putc(buffer[ret]);
//     }
//     return ret;
// }

// int sys_exec(const char *name, char *const argv[]){
//     void *current = get_cpio_base();
//     file_info_t info;
//     while(current != NULL){
//         if(cpio_file_iter(&current, &info)){    // abnormal iter
//             return -1;
//         }else if(!strcmp(name, info.name)){      // file name not match
            // load program and execute
    // void* target = kmalloc(file_size, 0);
    // memcpy(target, file_addr, file_size);
    // move_to_user_mode((unsigned long)target);
//             return 0;
//         }
//     }

//     return -1;
// }

// int sys_fork(void){
//     return 0;
// }

// void sys_exit(void){

// }

// int sys_mailbox_call(uint8_t channel, uint32_t *mbox){
//     return 0;
// }

// void sys_kill(int pid){

// }