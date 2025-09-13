#include "shell.h"
#include "common.h"
#include "config.h"
#include "mini_uart.h"
#include "power.h"

extern uint32_t kernel_size;
extern uint64_t dtb_ptr;
typedef void (*kernel_entrance_t)(uint64_t);

extern char kernel_begin;

static char buffer[SHELL_BUFFER_MAX_SIZE + 1] = {0};
static uint32_t buf_used = 0;

#define buf_remaining (SHELL_BUFFER_MAX_SIZE - buf_used)
#define err_code_to_msg(err_code) (~err_code)

static const char *err_msg[] = {
    "Reading error: command is too long.",
    "Empty command.",
};

#define ERR_CODE_MSG_TOO_LONG -1
#define ERR_CODE_EMPTY_COMMAND -2

static void new_line(void);
static int32_t read_command(void);
static void execute_command(void);

static void cmd_help(void);
static void cmd_reboot(void);
static void cmd_upload(void);
static void cmd_boot(void);

static void execute_command(void)
{
    if (!strcmp(buffer, "help")) {
        cmd_help();
    } else if (!strcmp(buffer, "reboot")) {
        cmd_reboot();
    } else if (!strcmp(buffer, "#upload_kernel$")) {
        cmd_upload();
    } else if (!strcmp(buffer, "boot")) {
        cmd_boot();
    } else {
        uart_puts("Unsupported command: ");
        uart_putln(buffer);
    }
}

void shell(void)
{
    int32_t err_code = 0;
    uart_putln(
        "\rWelcome to the OGC kernel loader. Use \"help\" for information on supported commands.");
    while (true) {
        new_line();
        if ((err_code = read_command())) {
            uart_putln(err_msg[err_code_to_msg(err_code)]);
            continue;
        }
        // length check
        if (buf_used == 0) {
            uart_putln(err_msg[err_code_to_msg(ERR_CODE_EMPTY_COMMAND)]);
            continue;
        }
        execute_command();
    }
}

static void new_line(void)
{
    uart_puts("\r$ ");
    buf_used = 0;
}

static int32_t read_command(void)
{
    char c;
    do {
        c = uart_getc();
        if (c == '\n') {  // enter new line
            buffer[buf_used] = '\0';
            uart_puts("\r\n");
            return 0;
        } else if (c == 0x7F) {  // c = del
            if (buf_used) {
                // print 2 backspaces to remove del and one char
                uart_putc((char) 0x8);
                uart_putc(' ');
                uart_putc((char) 0x8);
                buf_used--;
            }
        } else if (c > 0x1F) {
            buffer[buf_used++] = c;
            uart_putc(c);
        }
    } while (c != '\n' && buf_remaining);

    return ERR_CODE_MSG_TOO_LONG;
}

static void cmd_help(void)
{
    uart_putln("help\t: Display the help menu.");
    uart_putln("reboot\t: Reboot the system.");
    uart_putln("boot\t: Boot the downloaded kernel.");
}

static void cmd_reboot(void)
{
    uart_putln("Rebooting...");
    uart_putln("");
    power_reset(100);
}

static void cmd_upload(void)
{
    char *kernel = (char *) &kernel_begin;

    // Reply with "$download_kernel#\\n" to acknowledge request.
    uart_puts("$download_kernel#\n");

    // Receive 4-byte little-endian integer representing kernel image size.
    kernel_size = 0;
    for (int i = 0; i < 32; i += 8) {
        kernel_size |= ((uint32_t) uart_getb() << i);
    }
    // Reply with "$start_upload#\\n" to signal
    uart_puts("$start_upload#\n");

    // Receive kernel image data.
    for (uint32_t i = 0; i < kernel_size; i++) {
        kernel[i] = uart_getb();
    }
    // Return "$done#\\n" indicating transfer completion.
    uart_puts("$done#\n");
}

static void cmd_boot(void)
{
    if (kernel_size == 0) {
        uart_putln("No kernel image available.");
        return;
    }

    asm volatile("SEV");
    uart_putln("Transferring control to the kernel...");
    wait_msec(1000);
    kernel_entrance_t entrance = (kernel_entrance_t) &kernel_begin;
    entrance(dtb_ptr);
}