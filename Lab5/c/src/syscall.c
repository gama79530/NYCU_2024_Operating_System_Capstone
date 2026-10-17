#include "syscall.h"

#include "mini_uart.h"
#include "mailbox.h"
#include "thread.h"
#include "util.h"

/* Private types */

typedef enum {
    SYSCALL_GETPID = 0,
    SYSCALL_UART_READ = 1,
    SYSCALL_UART_WRITE = 2,
    SYSCALL_EXEC = 3,
    SYSCALL_FORK = 4,
    SYSCALL_EXIT = 5,
    SYSCALL_MBOX_CALL = 6,
    SYSCALL_KILL = 7,
} syscall_number_t;

/* Private function declarations */

/** Pass EL0 syscall arguments and retrieve the result through the register ABI. */
static long user_syscall(syscall_number_t number, uint64_t arg0, uint64_t arg1);
/** Check each byte of a user string before reading, including its terminating NUL. */
static bool user_string_valid(const char *text);

/* Function implementations */

static long user_syscall(syscall_number_t number, uint64_t arg0, uint64_t arg1)
{
    register uint64_t x0 asm("x0") = arg0;
    register uint64_t x1 asm("x1") = arg1;
    register uint64_t x8 asm("x8") = number;

    asm volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x8) : "memory", "cc");
    return (long) x0;
}

static bool user_string_valid(const char *text)
{
    uintptr_t address = (uintptr_t) text;

    while (thread_user_buffer_valid(address, 1)) {
        if (*(const char *) address == '\0') {
            return true;
        }
        address++;
    }
    return false;
}

int sys_getpid(void)
{
    return (int) user_syscall(SYSCALL_GETPID, 0, 0);
}

long sys_uart_read(char *buffer, size_t size)
{
    return user_syscall(SYSCALL_UART_READ, (uintptr_t) buffer, size);
}

long sys_uart_write(const char *buffer, size_t size)
{
    return user_syscall(SYSCALL_UART_WRITE, (uintptr_t) buffer, size);
}

int sys_exec(const char *name, char *const argv[])
{
    return (int) user_syscall(SYSCALL_EXEC, (uintptr_t) name, (uintptr_t) argv);
}

int sys_fork(void)
{
    return (int) user_syscall(SYSCALL_FORK, 0, 0);
}

_Noreturn void sys_exit(int status)
{
    /* Exit must not return; retry if the kernel unexpectedly returns. */
    while (true) {
        user_syscall(SYSCALL_EXIT, (uint64_t) status, 0);
    }
}

int sys_mbox_call(unsigned char channel, unsigned int *mbox)
{
    return (int) user_syscall(SYSCALL_MBOX_CALL, channel, (uintptr_t) mbox);
}

void sys_kill(int pid)
{
    user_syscall(SYSCALL_KILL, (uint64_t) pid, 0);
}

void syscall_dispatch(exception_frame_t *frame)
{
    switch (frame->regs[8]) {
    case SYSCALL_GETPID:
        frame->regs[0] = thread_current_id();
        break;
    case SYSCALL_UART_READ: {
        uintptr_t address = frame->regs[0];
        size_t size = frame->regs[1];
        char *buffer = (char *) address;

        if (!thread_user_buffer_valid(address, size)) {
            frame->regs[0] = (uint64_t) -1;
            break;
        }
        /* Thread context: UART waiting must allow IRQ and timer preemption. */
        for (size_t i = 0; i < size; i++) {
            buffer[i] = mini_uart_getc();
        }
        frame->regs[0] = size;
        break;
    }
    case SYSCALL_UART_WRITE: {
        uintptr_t address = frame->regs[0];
        size_t size = frame->regs[1];
        const char *buffer = (const char *) address;

        if (!thread_user_buffer_valid(address, size)) {
            frame->regs[0] = (uint64_t) -1;
            break;
        }
        /* Thread context: UART waiting must allow IRQ and timer preemption. */
        for (size_t i = 0; i < size; i++) {
            mini_uart_putc(buffer[i]);
        }
        frame->regs[0] = size;
        break;
    }
    case SYSCALL_EXEC: {
        const char *name = (const char *) frame->regs[0];

        /* Lab5 does not require argv passing. Success rewrites the return frame
         * to enter the new image; failure leaves the old program intact. */
        if (!user_string_valid(name) || thread_exec(name, frame) < 0) {
            frame->regs[0] = (uint64_t) -1;
        }
        break;
    }
    case SYSCALL_FORK:
        frame->regs[0] = (uint64_t) thread_fork(frame);
        break;
    case SYSCALL_EXIT:
        /* No wait/status API yet; x0's status is accepted but not retained. */
        thread_exit();
    case SYSCALL_MBOX_CALL: {
        uintptr_t address = frame->regs[1];
        uint32_t size;

        /* Property messages carry their byte size in word 0. Validate the size
         * field before reading it, then validate the entire 16-byte-aligned buffer. */
        if (frame->regs[0] != 8 || (address & 15) != 0 ||
            !thread_user_buffer_valid(address, sizeof(size))) {
            frame->regs[0] = (uint64_t) -1;
            break;
        }
        memcpy(&size, (const void *) address, sizeof(size));
        if (!thread_user_buffer_valid(address, size)) {
            frame->regs[0] = (uint64_t) -1;
            break;
        }
        frame->regs[0] = mailbox_call((unsigned char) frame->regs[0],
                                      (volatile uint32_t *) address) == MAILBOX_SUCCESS;
        break;
    }
    case SYSCALL_KILL: {
        int pid = (int32_t) (uint32_t) frame->regs[0];

        frame->regs[0] = thread_kill(pid) ? 0 : (uint64_t) -1;
        break;
    }
    default:
        frame->regs[0] = (uint64_t) -1;
        break;
    }
}
