#ifndef KERNEL_LOADER_MINI_UART_H
#define KERNEL_LOADER_MINI_UART_H
#include "types.h"

#define uart_init mini_uart_init
#define uart_getb mini_uart_getb
#define uart_getc mini_uart_getc
#define uart_putc mini_uart_putc
#define uart_puts mini_uart_puts
#define uart_putln mini_uart_putln

void mini_uart_init();
char mini_uart_getb();
char mini_uart_getc();
void mini_uart_putc(char c);
void mini_uart_puts(const char *s);
void mini_uart_putln(const char *s);

#endif
