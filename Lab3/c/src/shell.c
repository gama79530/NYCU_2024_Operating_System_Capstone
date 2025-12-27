#include "shell.h"
#include "common.h"
#include "config.h"
#include "initrd.h"
#include "mailbox.h"
#include "mini_uart.h"
#include "peripheral.h"
#include "power.h"
#include "printf.h"

static char buffer[SHELL_BUFFER_MAX_SIZE + 1] = {0};
static uint32_t buf_used = 0;
static char tokens[SHELL_TOKEN_MAX_NUM][SHELL_TOKEN_MAX_LEN + 1] = {0};
static uint32_t tokens_count = 0;

#define buf_remaining (SHELL_BUFFER_MAX_SIZE - buf_used)
#define err_code_to_msg(err_code) (~err_code)

static const char *err_msg[] = {
    "Reading error: command is too long.",
    "Parsing error: token length is too long.",
    "Empty command.",
    "Parsing error: too many tokens.",
};

#define ERR_CODE_MSG_TOO_LONG -1
#define ERR_CODE_TOO_LONG_TOKEN -2
#define ERR_CODE_EMPTY_COMMAND -3
#define ERR_CODE_TOO_MANY_TOKENS -4

static void new_line(void);
static int32_t read_command(void);
static int32_t parse_command(void);
static void execute_command(void);
static inline bool is_help(void);

static void cmd_help(void);
static void cmd_hello(void);
static void cmd_mailbox(void);
static void cmd_reboot(void);
static void cmd_ls(void);
static void cmd_cat(void);
static void cmd_svc_demo(void);

// cmd_mailbox
static void get_board_revision(void);
static void get_arm_memory_info(void);

static void execute_command(void)
{
    if (!strcmp(tokens[0], "help")) {
        cmd_help();
    } else if (!strcmp(tokens[0], "hello")) {
        cmd_hello();
    } else if (!strcmp(tokens[0], "mailbox")) {
        cmd_mailbox();
    } else if (!strcmp(tokens[0], "reboot")) {
        cmd_reboot();
    } else if (!strcmp(tokens[0], "ls")) {
        cmd_ls();
    } else if (!strcmp(tokens[0], "cat")) {
        cmd_cat();
    } else if (!strcmp(tokens[0], "svc_demo")) {
        cmd_svc_demo();
    } else {
        printf(
            "Unsupported command: %s\n"
            "\n",
            buffer);
    }
}

void shell(void)
{
    int32_t err_code = 0;
    printf("\rWelcome to the OGC shell. Use \"help\" for information on supported commands.\n");
    while (true) {
        new_line();
        if ((err_code = read_command())) {
            printf("\n%s\n", err_msg[err_code_to_msg(err_code)]);
            continue;
        }
        if ((err_code = parse_command())) {
            printf("\n%s\n", err_msg[err_code_to_msg(err_code)]);
            continue;
        }
        execute_command();
    }
}

static void new_line(void)
{
    printf("\r$ ");
    buf_used = 0;
}

static int32_t read_command(void)
{
    char c;
    do {
        c = uart_getc();
        if (c == '\n') {  // enter new line
            buffer[buf_used] = '\0';
            printf("\n");
            return 0;
        } else if (c == 0x7F) {  // c = del
            if (buf_used) {
                printf("%c %c", (char) 0x8,
                       (char) 0x8);  // print 2 backspaces to remove del and one char
                buf_used--;
            }
        } else if (c > 0x1F) {
            buffer[buf_used++] = c;
            printf("%c", c);
        }
    } while (c != '\n' && buf_remaining);

    return ERR_CODE_MSG_TOO_LONG;
}

static int32_t parse_command(void)
{
    const char *src = (const char *) buffer;
    char *dest;

    for (tokens_count = 0; *src != '\0' && tokens_count < SHELL_TOKEN_MAX_NUM; tokens_count++) {
        while (*src == ' ') {  // skip leading spaces before token
            src++;
        }

        dest = tokens[tokens_count];
        for (uint32_t token_len = 0; *src != ' ' && *src != '\0'; token_len++) {
            if (token_len >= SHELL_TOKEN_MAX_LEN) {
                return ERR_CODE_TOO_LONG_TOKEN;
            }
            *(dest++) = *(src++);
        }
        *(dest) = '\0';
    }

    if (tokens_count == 0) {
        return ERR_CODE_EMPTY_COMMAND;
    } else if (*src != '\0') {
        return ERR_CODE_TOO_MANY_TOKENS;
    }

    return 0;
}

static void cmd_help(void)
{
    printf(
        "The basic format of a command is \"{Command} [arg1 arg2 ...]\".\n"
        "Arguments are optional tokens: [arg], [-arg], or [-arg=xxx].\n"
        "Each token has a maximum length of %i characters, excluding the terminating null.\n"
        "Commands marked with '*' accept arguments.\n"
        "Use the argument \"-h\" or \"-help\" to find more details.\n"
        "\n"
        "help\t\t: Display the help menu.\n"
        "hello\t\t: Print \"Hello World!\"\n"
        "mailbox*\t: Communicate with the VideoCoreIV GPU.\n"
        "reboot\t\t: Reboot system.\n"
        "ls*\t\t: List all file names in ramdisk.\n"
        "cat*\t\t: Display file name and content.\n"
        "svc_demo*\t: Demonstration SVC instruction.\n",
        SHELL_TOKEN_MAX_LEN);
}

static void cmd_hello(void)
{
    printf(
        "Hello C kernel!\n"
        "\n");
}

static void cmd_mailbox(void)
{
    if (is_help()) {
        printf(
            "<default>\t\t: with all optional arguments.\n"
            "[-revision]\t\t: Display the board revision.\n"
            "[-arm-memory-info]\t: Display the ARM memory base address and size.\n"
            "\n");
        return;
    }

    if (tokens_count == 1) {
        get_board_revision();
        get_arm_memory_info();
    } else {
        for (int32_t i = 1; i < tokens_count; i++) {
            if (!strcmp(tokens[i], "-revision")) {
                get_board_revision();
            } else if (!strcmp(tokens[i], "-arm-memory-info")) {
                get_arm_memory_info();
            } else {
                printf("Unsupported argument: %s\n", tokens[i]);
            }
        }
    }
    printf("\n");
}

static void cmd_reboot(void)
{
    printf("\n");
    power_reset(100);
}

static void get_board_revision(void)
{
    volatile uint32_t *buffer = get_default_buffer();

    buffer[0] = 7 * 4;  // buffer size in bytes
    buffer[1] = MBOX_REQUEST;
    // tags begin
    buffer[2] = MBOX_TAG_GETREVISION;  // tag identifier
    buffer[3] = 4;                     // maximum of request and response value buffer's length.
    buffer[4] = MBOX_TAG_REQUEST;
    buffer[5] = 0;  // buffer for revision
    buffer[6] = MBOX_TAG_END;
    // tags end

    int32_t ret = mailbox_exchange(MBOX_CH_PROP, buffer);
    if (ret) {
        printf("Mailbox error: get_board_revision(), %s\n", err_code_to_str(ret));
    } else {
        printf("Board revision\t: 0x%X\n", buffer[5]);
    }
}

static void get_arm_memory_info(void)
{
    volatile uint32_t *buffer = get_default_buffer();
    buffer[0] = 8 * 4;
    buffer[1] = MBOX_REQUEST;
    // tags begin
    buffer[2] = MBOX_TAG_GETMEMORY;
    buffer[3] = 8;  // maximum of request and response value buffer's length.
    buffer[4] = MBOX_TAG_REQUEST;
    buffer[5] = 0;  // buffer for memory base address
    buffer[6] = 0;  // buffer for memory size
    buffer[7] = MBOX_TAG_END;
    // tags end

    int32_t ret = mailbox_exchange(MBOX_CH_PROP, buffer);
    if (ret) {
        printf("Mailbox error: get_arm_memory_info(), %s\n", err_code_to_str(ret));
    } else {
        printf("ARM memory base\t: 0x%08X\n", buffer[5]);
        printf("ARM memory size\t: 0x%08X\n", buffer[6]);
    }
}

static inline bool is_help(void)
{
    for (int32_t i = 1; i < tokens_count; i++) {
        if (!strcmp(tokens[i], "-h") || !strcmp(tokens[i], "-help")) {
            return true;
        }
    }
    return false;
}

static void cmd_ls(void)
{
    if (is_help()) {
        printf(
            "[default]\t: Display all file names in the ramdisk.\n"
            "{file name}\t: Check whether the file is in the ramdisk.\n"
            "\t\t  You can specify multiple files at once.\n");
        return;
    }

    const char *it = iter_begin();
    file_info_t info;

    while (true) {
        int state = iter_next(&it, &info);
        if (state == ITER_END) {
            break;
        } else if (state == ERR_MAGIC) {
            printf("%s\n", ERR_MAGIC_MSG);
            return;
        }

        if (tokens_count == 1) {  // list all file names
            printf("%s: %u bytes\n", info.name, info.content_size);
        } else {  // check whether the file is in the ramdisk
            for (int32_t i = 1; i < tokens_count; i++) {
                if (!strcmp(tokens[i], info.name)) {
                    printf("%s: %u bytes\n", info.name, info.content_size);
                }
            }
        }
    }
}

static void cmd_cat(void)
{
    if (is_help()) {
        printf(
            "[default]\t: Display all file names and their contents in the ramdisk.\n"
            "{file name}\t: Display the file name and its content if it is in the ramdisk.\n"
            "\t\t  You can specify multiple files at once.\n");
        return;
    }

    const char *it = iter_begin();
    file_info_t info;
    while (true) {
        int state = iter_next(&it, &info);
        if (state == ITER_END) {
            break;
        } else if (state == ERR_MAGIC) {
            printf("%s\n", ERR_MAGIC_MSG);
            return;
        }

        if (tokens_count == 1) {  // display all file names and their contents
            printf("%s\n", info.name);
            for (uint32_t i = 0; i < info.content_size; i++) {
                printf("%c", info.content[i]);
            }
            printf("\n");
        } else {  // display the file name and its content if it is in the ramdisk
            for (int32_t i = 1; i < tokens_count; i++) {
                if (!strcmp(tokens[i], info.name)) {
                    printf("%s\n", info.name);
                    for (uint32_t i = 0; i < info.content_size; i++) {
                        printf("%c", info.content[i]);
                    }
                    printf("\n");
                }
            }
        }
    }
}

static void cmd_svc_demo(void)
{
    const char *it = iter_begin();
    file_info_t info;

    // get user program from initrd
    while (true) {
        int state = iter_next(&it, &info);
        if (state == ITER_END) {
            break;
        } else if (state == ERR_MAGIC) {
            printf("%s\n", ERR_MAGIC_MSG);
            return;
        } else if (strcmp(info.name, "user.img")) {
            continue;
        }

        /*  0011 0100 0000
              ||  |   |
              ||  |   +--- M[3:0]: EL0t
              ||  +------- F[6]  : FIQ mask bit
              |+---------- A[8]  : SError mask bit
              +----------- D[9]  : Debug mask bit
        */
        const uint64_t spsr_el1 = 0x340;
        const uint64_t elr_el1 = 0x20000;
        const uint64_t sp = elr_el1 + 0x2000;
        char *user_prog = (char *) elr_el1;
        for (int i = 0; i < info.content_size; i++) {
            user_prog[i] = (char) info.content[i];
        }
        
        // Switch to EL0 and execute user program
        asm volatile("msr spsr_el1, %0" : : "r"(spsr_el1));
        asm volatile("msr elr_el1, %0" : : "r"(elr_el1));
        asm volatile("msr sp_el0, %0" : : "r"(sp));
        asm volatile("eret");
        break;
    }
}
