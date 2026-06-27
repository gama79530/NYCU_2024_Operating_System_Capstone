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
    /* enter simple shell */
    shell_run();
}

static void printf_putc(void *context, char c)
{
    (void) context;
    mini_uart_putc(c);
}
