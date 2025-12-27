#include "common.h"
#include "config.h"
#include "device_tree.h"
#include "initrd.h"
#include "mini_uart.h"
#include "printf.h"
#include "shell.h"

void kernel_service_init(uint64_t x0);
void putc(void *p, char c);

void main(uint64_t x0)
{
    kernel_service_init(x0);
    shell();
}

void kernel_service_init(uint64_t x0)
{
    int ret = 0;
    uart_init();
    init_printf(NULL, putc);

    if ((ret = set_dtb_address(x0))) {
        printf("%s\n", fdt_err_msg[~ret]);
        return;
    }

    if ((ret = set_initrd())) {
        printf("initrd error: %d\n", fdt_err_msg[~ret]);
        return;
    }
}

void putc(void *p, char c)
{
    if (c == '\n') {
        mini_uart_putc('\r');
    }
    uart_putc(c);
}
