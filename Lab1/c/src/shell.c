#include "shell.h"

#include "config.h"
#include "error.h"
#include "mini_uart.h"
#include "printf.h"
#include "string.h"
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
static const char *shell_error_string(shell_error_t error);
static void shell_print_error(shell_error_t error);
static shell_error_t shell_read_line(char *buffer, size_t capacity);
static shell_error_t shell_parse_args(char *line, char *argv[], size_t capacity, size_t *argc);
static const command_t *shell_find_command(const char *name);
static void shell_print_usage(const command_t *command);
static void shell_dispatch(size_t argc, char *argv[]);

/* command handlers */
static void cmd_help(size_t argc, char *argv[]);
static void cmd_hello(size_t argc, char *argv[]);

/* Private data */
static const command_t commands[] = {
    {"help", "help [command]", "List commands or show help for one command", cmd_help},
    {"hello", "hello", "Print Hello World!", cmd_hello},
};

static const char *const shell_error_messages[] = {
    "Command is too long.",
    "Too many arguments.",
};

#define COMMAND_COUNT (sizeof(commands) / sizeof(commands[0]))
#define SHELL_ERROR_COUNT (sizeof(shell_error_messages) / sizeof(shell_error_messages[0]))

/* Function implementations */
void shell_run(void)
{
    char buffer[CONFIG_SHELL_BUFFER_SIZE];
    char *argv[CONFIG_SHELL_MAX_ARGS];

    printf("Simple shell ready. Type \"help\" for available commands.\n");

    while (true) {
        size_t argc = 0;
        shell_error_t error;

        printf("$ ");

        error = shell_read_line(buffer, sizeof(buffer));
        if (error != SHELL_SUCCESS) {
            shell_print_error(error);
            continue;
        }

        error = shell_parse_args(buffer, argv, CONFIG_SHELL_MAX_ARGS, &argc);
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

static shell_error_t shell_read_line(char *buffer, size_t capacity)
{
    size_t buffer_length = 0;
    size_t display_length = 0;

    while (true) {
        char c = mini_uart_getc();

        if (c == '\n') {
            buffer[buffer_length] = '\0';
            printf("\n");
            return display_length >= capacity ? SHELL_ERROR_COMMAND_TOO_LONG : SHELL_SUCCESS;
        }

        if (c == '\b' || c == 0x7f) {
            if (display_length > 0) {
                display_length--;
                if (buffer_length > display_length) {
                    buffer_length--;
                }
                printf("\b \b");
            }
            continue;
        }

        if (c < ' ' || c > '~') {
            continue;
        }

        if (display_length + 1 < capacity) {
            buffer[buffer_length++] = c;
        }
        display_length++;
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
