#include "allocator.h"
#include "initramfs.h"
#include "mini_uart.h"
#include "printf.h"
#include "shell.h"
#include "types.h"

/* Private types */

/* Private function declarations */
static void printf_putc(void *context, char c);

/* Private data */

/* Function implementations */
void main(void)
{
    /* initialization */
    mini_uart_init();
    init_printf(NULL, printf_putc);
    simple_allocator_init();

    /*
     * Lab 2 basic: QEMU loads initramfs to 0x8000000 by default.
     * Later, the devicetree exercise will replace this hardcoded range with
     * linux,initrd-start and linux,initrd-end from /chosen.
     */
    initramfs_use_default_range();

    /* enter simple shell */
    shell_run();
}

static void printf_putc(void *context, char c)
{
    (void) context;
    mini_uart_putc(c);
}
