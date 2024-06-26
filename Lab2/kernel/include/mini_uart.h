#ifndef MINI_UART_H
#define MINI_UART_H

void uart_init(void);

char uart_getb(void);
char uart_getc(void);
void uart_putc(char c);

void uart_puts(const char *s);
void uart_putln(const char *s);
void uart_put_mutiln(const char *s, int len);

#endif