#ifndef LAB5_C_CONFIG_H
#define LAB5_C_CONFIG_H

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

#define CONFIG_SHELL_TIMEOUT_SECONDS 2
#define CONFIG_SHELL_TIMEOUT_MESSAGE "timeout"

#if CONFIG_SHELL_TIMEOUT_SECONDS < 1
#error "CONFIG_SHELL_TIMEOUT_SECONDS must be at least 1"
#endif

/* mailbox.c */
#define CONFIG_MAILBOX_TIMEOUT 1000000

#if CONFIG_MAILBOX_TIMEOUT < 1
#error "CONFIG_MAILBOX_TIMEOUT must be at least 1"
#endif

/* initramfs.c */
#define CONFIG_INITRAMFS_BASE 0x08000000UL
#define CONFIG_INITRAMFS_END 0x08200000UL

#if CONFIG_INITRAMFS_END <= CONFIG_INITRAMFS_BASE
#error "CONFIG_INITRAMFS_END must be greater than CONFIG_INITRAMFS_BASE"
#endif

/* Fallback physical memory range when the DTB does not provide one. */
#define CONFIG_BUDDY_FALLBACK_MEMORY_BASE 0x00000000UL
#define CONFIG_BUDDY_FALLBACK_MEMORY_END 0x3C000000UL
#define CONFIG_BUDDY_PAGE_SHIFT 12
#define CONFIG_BUDDY_MAX_ORDER 15

#if CONFIG_BUDDY_FALLBACK_MEMORY_END <= CONFIG_BUDDY_FALLBACK_MEMORY_BASE
#error "CONFIG_BUDDY_FALLBACK_MEMORY_END must be greater than CONFIG_BUDDY_FALLBACK_MEMORY_BASE"
#endif

#if CONFIG_BUDDY_PAGE_SHIFT >= 64
#error "CONFIG_BUDDY_PAGE_SHIFT must be less than 64"
#endif

#if CONFIG_BUDDY_PAGE_SHIFT < 8
#error "Dynamic allocator requires a buddy page size of at least 256 bytes"
#endif

#if CONFIG_BUDDY_MAX_ORDER < 6
#error "Lab 5 requires the buddy system maximum order to be greater than 5"
#endif

#if CONFIG_BUDDY_MAX_ORDER >= 64
#error \
    "CONFIG_BUDDY_MAX_ORDER must be less than 64: the 6-bit order field only encodes 0 through 63, and shifting 1UL by 64 is undefined"
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

/*
 * thread.c: allocation size for kernel calls and nested IRQ frames.
 * With the current allocator and 4 KiB pages, 15 KiB leaves room for the
 * allocation header within four pages. Usable stack bounds are aligned
 * inward; usable capacity may be smaller than this allocation size.
 */
#define CONFIG_THREAD_STACK_SIZE (15 * 1024)

/*
 * The 4 KiB minimum is a conservative project policy, not an ARM requirement.
 * It rejects very small stacks but does not guarantee against overflow;
 * required capacity depends on call depth, local variables, and IRQ nesting.
 * Keep the allocation size in 16-byte units. With malloc's 8-byte alignment,
 * aligning both bounds inward trims either zero or 16 bytes in total.
 */
#if CONFIG_THREAD_STACK_SIZE < 4096 || (CONFIG_THREAD_STACK_SIZE % 16) != 0
#error "CONFIG_THREAD_STACK_SIZE must be at least 4096 and a multiple of 16"
#endif

/* Includes reserved IDs 0 (boot) and 1 (idle). */
#ifndef CONFIG_THREAD_ID_COUNT
#define CONFIG_THREAD_ID_COUNT 256
#endif

#if CONFIG_THREAD_ID_COUNT < 3 || CONFIG_THREAD_ID_COUNT > 65536
#error "CONFIG_THREAD_ID_COUNT must be between 3 and 65536"
#endif

/* Maximum zombies reclaimed during one idle turn before yielding. */
#ifndef CONFIG_THREAD_REAP_LIMIT
#define CONFIG_THREAD_REAP_LIMIT 1
#endif

#if CONFIG_THREAD_REAP_LIMIT < 1
#error "CONFIG_THREAD_REAP_LIMIT must be at least 1"
#endif

/* task_queue.c */
#define CONFIG_TASK_QUEUE_MAX_TASKS 64
#define CONFIG_TASK_QUEUE_CACHE_SIZE 16

#if CONFIG_TASK_QUEUE_MAX_TASKS < 1
#error "CONFIG_TASK_QUEUE_MAX_TASKS must be at least 1"
#endif

#if CONFIG_TASK_QUEUE_CACHE_SIZE < 0
#error "CONFIG_TASK_QUEUE_CACHE_SIZE must not be negative"
#endif

#if CONFIG_TASK_QUEUE_CACHE_SIZE > CONFIG_TASK_QUEUE_MAX_TASKS
#error "CONFIG_TASK_QUEUE_CACHE_SIZE must not exceed CONFIG_TASK_QUEUE_MAX_TASKS"
#endif

/* timer.c */
#define CONFIG_TIMER_MAX_EVENTS 64
#define CONFIG_TIMER_EVENT_CACHE_SIZE 16

#if CONFIG_TIMER_MAX_EVENTS < 1
#error "CONFIG_TIMER_MAX_EVENTS must be at least 1"
#endif

#if CONFIG_TIMER_EVENT_CACHE_SIZE < 0
#error "CONFIG_TIMER_EVENT_CACHE_SIZE must not be negative"
#endif

#if CONFIG_TIMER_EVENT_CACHE_SIZE > CONFIG_TIMER_MAX_EVENTS
#error "CONFIG_TIMER_EVENT_CACHE_SIZE must not exceed CONFIG_TIMER_MAX_EVENTS"
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
