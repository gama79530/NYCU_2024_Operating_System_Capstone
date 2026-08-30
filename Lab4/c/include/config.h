#ifndef LAB4_C_CONFIG_H
#define LAB4_C_CONFIG_H

#ifndef CONFIG_VERBOSE
#define CONFIG_VERBOSE 0
#endif

#if CONFIG_VERBOSE != 0 && CONFIG_VERBOSE != 1
#error "CONFIG_VERBOSE must be 0 or 1"
#endif

/* shell.c */
#define CONFIG_SHELL_BUFFER_SIZE 128
#define CONFIG_SHELL_MAX_ARGS 8

#if CONFIG_SHELL_BUFFER_SIZE < 2
#error "CONFIG_SHELL_BUFFER_SIZE must be at least 2"
#endif

#if CONFIG_SHELL_MAX_ARGS < 1
#error "CONFIG_SHELL_MAX_ARGS must be at least 1"
#endif

/* mailbox.c */
#define CONFIG_MAILBOX_TIMEOUT 1000000

#if CONFIG_MAILBOX_TIMEOUT < 1
#error "CONFIG_MAILBOX_TIMEOUT must be at least 1"
#endif

/* initramfs.c */
#define CONFIG_INITRAMFS_BASE 0x08000000UL
#define CONFIG_INITRAMFS_END  0x08200000UL

#if CONFIG_INITRAMFS_END <= CONFIG_INITRAMFS_BASE
#error "CONFIG_INITRAMFS_END must be greater than CONFIG_INITRAMFS_BASE"
#endif

/* Physical memory managed by the Lab 4 buddy system. */
#define CONFIG_BUDDY_MEMORY_BASE 0x00000000UL
#define CONFIG_BUDDY_MEMORY_END  0x3C000000UL
#define CONFIG_BUDDY_PAGE_SHIFT 12
#define CONFIG_BUDDY_MAX_ORDER  15

#if CONFIG_BUDDY_MEMORY_END <= CONFIG_BUDDY_MEMORY_BASE
#error "CONFIG_BUDDY_MEMORY_END must be greater than CONFIG_BUDDY_MEMORY_BASE"
#endif

#if CONFIG_BUDDY_PAGE_SHIFT >= 64
#error "CONFIG_BUDDY_PAGE_SHIFT must be less than 64"
#endif

#if CONFIG_BUDDY_MAX_ORDER < 6
#error "Lab 4 requires the buddy system maximum order to be greater than 5"
#endif

#if CONFIG_BUDDY_MAX_ORDER >= 64
#error "CONFIG_BUDDY_MAX_ORDER must be less than 64: the 6-bit order field only encodes 0 through 63, and shifting 1UL by 64 is undefined"
#endif

#if CONFIG_BUDDY_PAGE_SHIFT + CONFIG_BUDDY_MAX_ORDER >= 64
#error "The largest buddy block must fit in uintptr_t"
#endif

/* fdt.c */
#define CONFIG_FDT_MAX_DEPTH 32

#if CONFIG_FDT_MAX_DEPTH < 1
#error "CONFIG_FDT_MAX_DEPTH must be at least 1"
#endif

/* power.c */
#define CONFIG_REBOOT_TICKS 100

#if CONFIG_REBOOT_TICKS < 1 || CONFIG_REBOOT_TICKS > 0x000FFFFF
#error "CONFIG_REBOOT_TICKS must fit the watchdog time field"
#endif

/* task_queue.c */
#define CONFIG_TASK_QUEUE_MAX_TASKS 64

#if CONFIG_TASK_QUEUE_MAX_TASKS < 1
#error "CONFIG_TASK_QUEUE_MAX_TASKS must be at least 1"
#endif

/* timer.c */
#define CONFIG_TIMER_MAX_EVENTS 64
/*
 * Keep timer_event_t at 128 bytes on AArch64:
 * list_head_t(16) + expires_at(8) + callback(8) + message(96).
 */
#define CONFIG_TIMER_MESSAGE_SIZE 96
#define CONFIG_TIMER_DEFAULT_TIMEOUT_SECONDS 2
#define CONFIG_TIMER_DEFAULT_MESSAGE "timeout"

#if CONFIG_TIMER_MAX_EVENTS < 1
#error "CONFIG_TIMER_MAX_EVENTS must be at least 1"
#endif

#if CONFIG_TIMER_MESSAGE_SIZE < 1
#error "CONFIG_TIMER_MESSAGE_SIZE must be at least 1"
#endif

#if CONFIG_TIMER_DEFAULT_TIMEOUT_SECONDS < 1
#error "CONFIG_TIMER_DEFAULT_TIMEOUT_SECONDS must be at least 1"
#endif

/* mini_uart.c */
/* Power-of-two ring storage; one slot remains unused to distinguish full from empty. */
#define CONFIG_MINI_UART_RX_BUFFER_SIZE 256
#define CONFIG_MINI_UART_TX_BUFFER_SIZE 256

#if CONFIG_MINI_UART_RX_BUFFER_SIZE < 2
#error "CONFIG_MINI_UART_RX_BUFFER_SIZE must be at least 2"
#endif

#if (CONFIG_MINI_UART_RX_BUFFER_SIZE & (CONFIG_MINI_UART_RX_BUFFER_SIZE - 1)) != 0
#error "CONFIG_MINI_UART_RX_BUFFER_SIZE must be a power of two"
#endif

#if CONFIG_MINI_UART_TX_BUFFER_SIZE < 2
#error "CONFIG_MINI_UART_TX_BUFFER_SIZE must be at least 2"
#endif

#if (CONFIG_MINI_UART_TX_BUFFER_SIZE & (CONFIG_MINI_UART_TX_BUFFER_SIZE - 1)) != 0
#error "CONFIG_MINI_UART_TX_BUFFER_SIZE must be a power of two"
#endif

/* EL0 SVC demo */
#define CONFIG_EL0_USER_ENTRY 0x00020000UL
#define CONFIG_EL0_USER_STACK 0x00022000UL
#define CONFIG_EL0_USER_IMAGE "user.img"

#endif
