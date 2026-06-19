#include "mini_uart.h"

void main(void)
{
    mini_uart_init();
    mini_uart_putln("hello world");
}
