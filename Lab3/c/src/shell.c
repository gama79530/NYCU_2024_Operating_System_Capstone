#include "shell.h"

#include "allocator.h"
#include "config.h"
#include "el.h"
#include "error.h"
#include "fdt.h"
#include "initramfs.h"
#include "mailbox.h"
#include "mini_uart.h"
#include "power.h"
#include "printf.h"
#include "string.h"
#include "timer.h"
#include "types.h"

/* Private types */
typedef void (*command_handler_t)(size_t argc, char *argv[]);

typedef struct {
    const char *name;
    const char *usage;
    const char *description;
    command_handler_t handler;
} command_t;

typedef enum {
    SHELL_SUCCESS = 0,
    SHELL_ERROR_COMMAND_TOO_LONG = -1,
    SHELL_ERROR_TOO_MANY_ARGS = -2,
} shell_error_t;

/* Private function declarations */
/* Convert a shell_error_t value into a user-facing error message. */
static const char *shell_error_string(shell_error_t error);

/* Print a shell_error_t using the common "Error:" prefix. */
static void shell_print_error(shell_error_t error);

/* Clear the current terminal line based on the number of visible characters. */
static void shell_clear_current_line(size_t length);

/* Clear the prompt line using the current buffered input length. */
static void shell_clear_prompt_line(size_t buffer_length);

/* Redraw the prompt and the buffered input line. */
static void shell_redraw_prompt(const char *buffer);

/* Print a timeout message without losing the current input line. */
static void shell_print_timeout_message(const char *message, uint64_t current_seconds);

/*
 * Read one line from Mini UART into buffer.
 *
 * Printable characters that would exceed capacity are rejected immediately.
 * The current buffered line is redrawn so the user can keep editing it.
 */
static shell_error_t shell_read_line(char *buffer, size_t capacity);

/*
 * Split a command line into space-separated arguments in place.
 *
 * Spaces are replaced with '\0', and argv points into the original line
 * buffer.
 */
static shell_error_t shell_parse_args(char *line, char *argv[], size_t capacity, size_t *argc);

/* Parse a decimal size argument used by shell commands. */
static bool shell_parse_size(const char *str, size_t *value);

/* Find a command descriptor by command name. */
static const command_t *shell_find_command(const char *name);

/* Print the usage string for a command descriptor. */
static void shell_print_usage(const command_t *command);

/* Dispatch argc/argv to a registered shell command handler. */
static void shell_dispatch(size_t argc, char *argv[]);

/* Private command declarations */
/* Print command help or the usage/details for one command. */
static void cmd_help(size_t argc, char *argv[]);

/* Print a simple greeting used to verify basic shell command dispatch. */
static void cmd_hello(size_t argc, char *argv[]);

/* Print Raspberry Pi mailbox information selected by command arguments. */
static void cmd_mailbox(size_t argc, char *argv[]);

/* Reboot the board through the watchdog power controller. */
static void cmd_reboot(size_t argc, char *argv[]);

/* List files from the current initramfs archive. */
static void cmd_ls(size_t argc, char *argv[]);

/* Print one file from the current initramfs archive. */
static void cmd_cat(size_t argc, char *argv[]);

/* Print the current simple heap allocator range and remaining space. */
static void cmd_heap(size_t argc, char *argv[]);

/* Allocate bytes from the simple heap for allocator testing. */
static void cmd_alloc(size_t argc, char *argv[]);

/* Print devicetree and initramfs range information discovered at boot. */
static void cmd_dtb(size_t argc, char *argv[]);

/* Add a one-shot timeout with optional message and seconds arguments. */
static void cmd_set_timeout(size_t argc, char *argv[]);

/*
 * Load an EL0 user program from initramfs and enter it.
 *
 * The default program is the Lab3 user.img SVC demo linked at
 * CONFIG_EL0_USER_ENTRY.
 */
static void cmd_svc(size_t argc, char *argv[]);

/* Private data */
#define SHELL_PROMPT "$ "

static const fdt_t *shell_fdt;
static char shell_line_buffer[CONFIG_SHELL_BUFFER_SIZE];
static size_t shell_line_length;
static bool shell_input_active;

static const command_t commands[] = {
    {"help", "help [command]", "List commands or show help for one command", cmd_help},
    {"hello", "hello", "Print Hello World!", cmd_hello},
    {"mailbox", "mailbox [revision|memory]...", "Print all or selected hardware information", cmd_mailbox},
    {"reboot", "reboot", "Reboot the Raspberry Pi", cmd_reboot},
    {"ls", "ls", "List files in the initramfs archive", cmd_ls},
    {"cat", "cat <path>", "Print a file from the initramfs archive", cmd_cat},
    {"heap", "heap", "Print simple allocator state", cmd_heap},
    {"alloc", "alloc <bytes>", "Allocate bytes from the simple allocator", cmd_alloc},
    {"dtb", "dtb", "Print devicetree initramfs information", cmd_dtb},
    {"setTimeout", "setTimeout [message] [seconds]", "Print a message after a timeout", cmd_set_timeout},
    {"svc", "svc [path]", "Run an EL0 user program from initramfs", cmd_svc},
};

static const char *const shell_error_messages[] = {
    "Command is too long.",
    "Too many arguments.",
};

#define COMMAND_COUNT (sizeof(commands) / sizeof(commands[0]))
#define SHELL_ERROR_COUNT (sizeof(shell_error_messages) / sizeof(shell_error_messages[0]))

/* Function implementations */
void shell_run(const fdt_t *fdt)
{
    char *argv[CONFIG_SHELL_MAX_ARGS];

    shell_fdt = fdt;
    printf("Simple shell ready. Type \"help\" for available commands.\n");

    while (true) {
        size_t argc = 0;
        shell_error_t error;

        printf(SHELL_PROMPT);

        shell_input_active = true;
        error = shell_read_line(shell_line_buffer, sizeof(shell_line_buffer));
        shell_input_active = false;
        if (error != SHELL_SUCCESS) {
            shell_print_error(error);
            continue;
        }

        error = shell_parse_args(shell_line_buffer, argv, CONFIG_SHELL_MAX_ARGS, &argc);
        if (error != SHELL_SUCCESS) {
            shell_print_error(error);
            continue;
        }

        if (argc != 0) {
            shell_dispatch(argc, argv);
        }
    }
}

static const char *shell_error_string(shell_error_t error)
{
    if (error >= SHELL_SUCCESS) {
        return "Unknown shell error.";
    }

    size_t index = error_code_to_index(error);

    if (index >= SHELL_ERROR_COUNT) {
        return "Unknown shell error.";
    }

    return shell_error_messages[index];
}

static void shell_print_error(shell_error_t error)
{
    printf("Error: %s\n", shell_error_string(error));
}

static void shell_clear_current_line(size_t length)
{
    printf("\r");
    for (size_t i = 0; i < length; i++) {
        printf(" ");
    }
    printf("\r");
}

static void shell_clear_prompt_line(size_t buffer_length)
{
    shell_clear_current_line(sizeof(SHELL_PROMPT) - 1 + buffer_length);
}

static void shell_redraw_prompt(const char *buffer)
{
    printf(SHELL_PROMPT "%s", buffer);
}

static void shell_print_timeout_message(const char *message, uint64_t current_seconds)
{
    if (shell_input_active) {
        shell_clear_prompt_line(shell_line_length);
    }

    printf("[Timeout at %u seconds since booting]: %s\n",
           (unsigned int) current_seconds,
           message);

    if (shell_input_active) {
        shell_redraw_prompt(shell_line_buffer);
    }
}

static shell_error_t shell_read_line(char *buffer, size_t capacity)
{
    shell_line_length = 0;
    buffer[0] = '\0';

    while (true) {
        char c = mini_uart_getc();

        if (c == '\n') {
            buffer[shell_line_length] = '\0';
            printf("\n");
            return SHELL_SUCCESS;
        }

        if (c == '\b' || c == 0x7f) {
            if (shell_line_length > 0) {
                shell_line_length--;
                buffer[shell_line_length] = '\0';
                printf("\b \b");
            }
            continue;
        }

        if (c < ' ' || c > '~') {
            continue;
        }

        if (shell_line_length + 1 >= capacity) {
            buffer[shell_line_length] = '\0';
            shell_clear_prompt_line(shell_line_length);
            shell_print_error(SHELL_ERROR_COMMAND_TOO_LONG);
            shell_redraw_prompt(buffer);
            continue;
        }

        buffer[shell_line_length++] = c;
        buffer[shell_line_length] = '\0';
        printf("%c", c);
    }
}

static shell_error_t shell_parse_args(char *line, char *argv[], size_t capacity, size_t *argc)
{
    char *cursor = line;
    *argc = 0;

    while (*cursor != '\0') {
        while (*cursor == ' ') {
            cursor++;
        }

        if (*cursor == '\0') {
            break;
        }

        if (*argc >= capacity) {
            return SHELL_ERROR_TOO_MANY_ARGS;
        }

        argv[(*argc)++] = cursor;

        while (*cursor != '\0' && *cursor != ' ') {
            cursor++;
        }

        if (*cursor != '\0') {
            *cursor++ = '\0';
        }
    }

    return SHELL_SUCCESS;
}

static bool shell_parse_size(const char *str, size_t *value)
{
    size_t length = 0;
    unsigned long parsed;

    while (str[length] != '\0') {
        length++;
    }

    if (length == 0 || !strntoul(str, length, 10, &parsed)) {
        return false;
    }

    *value = (size_t) parsed;
    return true;
}

static const command_t *shell_find_command(const char *name)
{
    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        if (strcmp(name, commands[i].name) == 0) {
            return &commands[i];
        }
    }

    return NULL;
}

static void shell_print_usage(const command_t *command)
{
    printf("Usage: %s\n", command->usage);
}

static void shell_dispatch(size_t argc, char *argv[])
{
    const command_t *command = shell_find_command(argv[0]);

    if (command != NULL) {
        command->handler(argc, argv);
        return;
    }

    printf("Unknown command: %s\n", argv[0]);
}

static void cmd_help(size_t argc, char *argv[])
{
    if (argc > 2) {
        shell_print_usage(shell_find_command(argv[0]));
        return;
    }

    if (argc == 1) {
        for (size_t i = 0; i < COMMAND_COUNT; i++) {
            printf("%s - %s\n", commands[i].name, commands[i].description);
        }
        return;
    }

    const command_t *command = shell_find_command(argv[1]);

    if (command == NULL) {
        printf("Unknown command: %s\n", argv[1]);
        return;
    }

    shell_print_usage(command);
    printf("%s\n", command->description);
}

static void cmd_hello(size_t argc, char *argv[])
{
    (void) argv;

    if (argc != 1) {
        shell_print_usage(shell_find_command(argv[0]));
        return;
    }

    printf("Hello World!\n");
}

static void cmd_mailbox(size_t argc, char *argv[])
{
    bool show_revision = argc == 1;
    bool show_memory = argc == 1;
    uint32_t revision;
    uint32_t memory_base;
    uint32_t memory_size;
    mailbox_error_t error;

    for (size_t i = 1; i < argc; i++) {
        if (strcmp(argv[i], "revision") == 0) {
            show_revision = true;
        } else if (strcmp(argv[i], "memory") == 0) {
            show_memory = true;
        } else {
            printf("Unknown mailbox query: %s\n", argv[i]);
            return;
        }
    }

    if (show_revision) {
        error = mailbox_get_board_revision(&revision);
        if (error != MAILBOX_SUCCESS) {
            printf("Mailbox error: %s\n", mailbox_error_string(error));
            return;
        }

        printf("Board revision : 0x%08X\n", revision);
    }

    if (show_memory) {
        error = mailbox_get_arm_memory(&memory_base, &memory_size);
        if (error != MAILBOX_SUCCESS) {
            printf("Mailbox error: %s\n", mailbox_error_string(error));
            return;
        }

        printf("ARM memory base: 0x%08X\n", memory_base);
        printf("ARM memory size: 0x%08X\n", memory_size);
    }
}

static void cmd_reboot(size_t argc, char *argv[])
{
    if (argc != 1) {
        shell_print_usage(shell_find_command(argv[0]));
        return;
    }

    printf("Rebooting...\n");
    power_reboot();
}

static void cmd_ls(size_t argc, char *argv[])
{
    const uint8_t *cursor;
    initramfs_file_t file;
    initramfs_error_t error;

    if (argc != 1) {
        shell_print_usage(shell_find_command(argv[0]));
        return;
    }

    cursor = initramfs_begin();
    while ((error = initramfs_next(&cursor, &file)) == INITRAMFS_SUCCESS) {
        if (strcmp(".", file.name) == 0) {
            continue;
        }

        printf("%s (%u bytes)\n", file.name, (unsigned int) file.size);
    }

    if (error != INITRAMFS_END) {
        printf("Initramfs error: %s\n", initramfs_error_string(error));
    }
}

static void cmd_cat(size_t argc, char *argv[])
{
    const uint8_t *cursor;
    initramfs_file_t file;
    initramfs_error_t error;

    if (argc != 2) {
        shell_print_usage(shell_find_command(argv[0]));
        return;
    }

    cursor = initramfs_begin();
    while ((error = initramfs_next(&cursor, &file)) == INITRAMFS_SUCCESS) {
        if (!initramfs_path_matches(argv[1], file.name)) {
            continue;
        }

        for (size_t i = 0; i < file.size; i++) {
            printf("%c", file.data[i]);
        }
        if (file.size == 0 || file.data[file.size - 1] != '\n') {
            printf("\n");
        }
        return;
    }

    if (error != INITRAMFS_END) {
        printf("Initramfs error: %s\n", initramfs_error_string(error));
        return;
    }

    printf("File not found: %s\n", argv[1]);
}

static void cmd_heap(size_t argc, char *argv[])
{
    if (argc != 1) {
        shell_print_usage(shell_find_command(argv[0]));
        return;
    }

    printf("heap begin    : 0x%08X\n", (unsigned int) simple_allocator_begin());
    printf("heap current  : 0x%08X\n", (unsigned int) simple_allocator_current());
    printf("heap end      : 0x%08X\n", (unsigned int) simple_allocator_end());
    printf("heap remaining: %u bytes\n", (unsigned int) simple_allocator_remaining());
}

static void cmd_alloc(size_t argc, char *argv[])
{
    size_t size;
    void *ptr;

    if (argc != 2) {
        shell_print_usage(shell_find_command(argv[0]));
        return;
    }

    if (!shell_parse_size(argv[1], &size)) {
        printf("Invalid size: %s\n", argv[1]);
        return;
    }

    ptr = simple_malloc(size);
    if (ptr == NULL) {
        printf("Allocation failed: %u bytes\n", (unsigned int) size);
        return;
    }

    printf("Allocated %u bytes at 0x%08X\n", (unsigned int) size, (unsigned int) (uintptr_t) ptr);
}

static void cmd_dtb(size_t argc, char *argv[])
{
    fdt_error_t error;
    uintptr_t initramfs_begin;
    uintptr_t initramfs_end;

    if (argc != 1) {
        shell_print_usage(shell_find_command(argv[0]));
        return;
    }

    if (shell_fdt == NULL) {
        printf("FDT error : %s\n", fdt_error_string(FDT_ERROR_INVALID_ARGUMENT));
        return;
    }

    printf("dtb address: 0x%08X\n", (unsigned int) (uintptr_t) shell_fdt->base);

    error = initramfs_read_range_from_fdt(shell_fdt, &initramfs_begin, &initramfs_end);
    if (error != FDT_SUCCESS) {
        printf("initramfs : %s\n", fdt_error_string(error));
        return;
    }

    printf("initrd start: 0x%08X\n", (unsigned int) initramfs_begin);
    printf("initrd end  : 0x%08X\n", (unsigned int) initramfs_end);
}

static void cmd_set_timeout(size_t argc, char *argv[])
{
    const char *message = CONFIG_TIMER_DEFAULT_MESSAGE;
    size_t seconds = CONFIG_TIMER_DEFAULT_TIMEOUT_SECONDS;

    if (argc > 3) {
        shell_print_usage(shell_find_command(argv[0]));
        return;
    }

    if (argc >= 2) {
        message = argv[1];
    }

    if (argc == 3 && !shell_parse_size(argv[2], &seconds)) {
        printf("Invalid timeout seconds: %s\n", argv[2]);
        return;
    }

    printf("<Timer>: current time: %u seconds since booting.\n",
           (unsigned int) timer_current_seconds());

    if (!timer_add_timeout((uint64_t) seconds, message, shell_print_timeout_message)) {
        printf("Timer error: failed to add timeout.\n");
    }
}

static void cmd_svc(size_t argc, char *argv[])
{
    const char *path = CONFIG_EL0_USER_IMAGE;
    const uint8_t *cursor;
    initramfs_file_t file;
    initramfs_error_t error;
    uint8_t *entry = (uint8_t *) CONFIG_EL0_USER_ENTRY;

    if (argc > 2) {
        shell_print_usage(shell_find_command(argv[0]));
        return;
    }

    if (argc == 2) {
        path = argv[1];
    }

    cursor = initramfs_begin();
    while ((error = initramfs_next(&cursor, &file)) == INITRAMFS_SUCCESS) {
        if (!initramfs_path_matches(path, file.name)) {
            continue;
        }

        for (size_t i = 0; i < file.size; i++) {
            entry[i] = file.data[i];
        }

        printf("Loaded %s to 0x%08X (%u bytes)\n",
               path,
               (unsigned int) CONFIG_EL0_USER_ENTRY,
               (unsigned int) file.size);
        printf("Entering EL0 user program.\n");
        el_enter_el0(CONFIG_EL0_USER_ENTRY, CONFIG_EL0_USER_STACK);
        return;
    }

    if (error != INITRAMFS_END) {
        printf("Initramfs error: %s\n", initramfs_error_string(error));
        return;
    }

    printf("File not found: %s\n", path);
}
