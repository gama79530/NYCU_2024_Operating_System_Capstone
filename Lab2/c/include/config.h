#ifndef LAB2_C_CONFIG_H
#define LAB2_C_CONFIG_H

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

/* power.c */
#define CONFIG_REBOOT_TICKS 100

#if CONFIG_REBOOT_TICKS < 1 || CONFIG_REBOOT_TICKS > 0x000FFFFF
#error "CONFIG_REBOOT_TICKS must fit the watchdog time field"
#endif

#endif
