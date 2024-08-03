#ifndef MINI_UART_H
#define MINI_UART_H
#include "types.h"

/* setting */
void uart_init(void);

/* polling io */
char uart_getb(void);
char uart_getc(void);
void uart_putc(char c);

void uart_puts(const char *s);
void uart_putln(const char *s);
void uart_put_mutiln(const char *s, uint32_t len);

#endif