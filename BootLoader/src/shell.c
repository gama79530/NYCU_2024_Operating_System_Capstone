#include "shell.h"

#include "config.h"
#include "mini_uart.h"
#include "string.h"
#include "util.h"

#define SHELL_BUFFER_SIZE 64

typedef void (*kernel_entry_t)(uint64_t dtb_addr);

static char command_buffer[SHELL_BUFFER_SIZE];
static uint32_t kernel_size;
static uint64_t kernel_dtb_addr;

static uint32_t read_u32_le(void)
{
    uint32_t value = 0;

    for (uint32_t shift = 0; shift < 32; shift += 8) {
        value |= (uint32_t) mini_uart_getb() << shift;
    }

    return value;
}

static void put_hex32(uint32_t value)
{
    static const char digits[] = "0123456789abcdef";

    mini_uart_puts("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        mini_uart_putc(digits[(value >> shift) & 0xf]);
    }
}

static void print_prompt(void)
{
    mini_uart_puts("(bootloader)$ ");
}

static bool read_line(void)
{
    size_t used = 0;

    while (true) {
        char c = mini_uart_getc();

        if (c == '\n') {
            command_buffer[used] = '\0';
            mini_uart_putc('\n');
            return true;
        }

        if (c == '\b' || c == 0x7f) {
            if (used > 0) {
                used--;
                mini_uart_puts("\b \b");
            }
            continue;
        }

        if (c < 0x20 || c > 0x7e) {
            continue;
        }

        if (used + 1 >= SHELL_BUFFER_SIZE) {
            command_buffer[used] = '\0';
            mini_uart_putln("");
            mini_uart_putln("error: command is too long");
            return false;
        }

        command_buffer[used++] = c;
        mini_uart_putc(c);
    }
}

static void receive_kernel(void)
{
    uint32_t magic;
    uint8_t *kernel = (uint8_t *) CONFIG_KERNEL_LOAD_ADDR;

    mini_uart_putln("$ready#");

    magic = read_u32_le();
    if (magic != CONFIG_BOOT_MAGIC) {
        mini_uart_putln("$bad_magic#");
        kernel_size = 0;
        return;
    }

    kernel_size = read_u32_le();
    if (kernel_size == 0 || kernel_size > CONFIG_KERNEL_MAX_SIZE) {
        mini_uart_puts("$bad_size:");
        put_hex32(kernel_size);
        mini_uart_putln("#");
        kernel_size = 0;
        return;
    }

    mini_uart_putln("$start#");
    for (uint32_t i = 0; i < kernel_size; i++) {
        kernel[i] = mini_uart_getb();
    }

    sync_instruction_cache(CONFIG_KERNEL_LOAD_ADDR, kernel_size);
    mini_uart_putln("$done#");
}

static void boot_kernel(void)
{
    kernel_entry_t kernel;

    if (kernel_size == 0) {
        mini_uart_putln("No kernel image loaded.");
        return;
    }

    mini_uart_puts("Booting kernel, size = ");
    put_hex32(kernel_size);
    mini_uart_putln("");

    data_sync_barrier();
    instruction_sync_barrier();

    kernel = (kernel_entry_t) CONFIG_KERNEL_LOAD_ADDR;
    kernel(kernel_dtb_addr);

    while (true) {
        asm volatile("wfe");
    }
}

static void print_help(void)
{
    mini_uart_putln("help   : Show this help message");
    mini_uart_putln("upload : Receive a kernel image over UART");
    mini_uart_putln("boot   : Boot the loaded kernel image");
}

static void execute_command(void)
{
    if (command_buffer[0] == '\0') {
        return;
    }

    if (strcmp("help", command_buffer) == 0) {
        print_help();
    } else if (strcmp("upload", command_buffer) == 0) {
        receive_kernel();
    } else if (strcmp("boot", command_buffer) == 0) {
        boot_kernel();
    } else {
        mini_uart_puts("Unknown command: ");
        mini_uart_putln(command_buffer);
    }
}

void shell_run(uint64_t dtb_addr)
{
    kernel_size = 0;
    kernel_dtb_addr = dtb_addr;

    mini_uart_putln("");
    mini_uart_putln("NYCU OSC bootloader shell");
    mini_uart_putln("Type \"help\" for commands.");

    while (true) {
        print_prompt();
        if (read_line()) {
            execute_command();
        }
    }
}
