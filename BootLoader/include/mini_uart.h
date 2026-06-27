#ifndef BOOTLOADER_MINI_UART_H
#define BOOTLOADER_MINI_UART_H

#include "types.h"

void mini_uart_init(void);
uint8_t mini_uart_getb(void);
char mini_uart_getc(void);
void mini_uart_putc(char c);
void mini_uart_puts(const char *s);
void mini_uart_putln(const char *s);

#endif
