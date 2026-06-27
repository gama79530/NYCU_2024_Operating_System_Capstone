#ifndef LAB2_C_MINI_UART_H
#define LAB2_C_MINI_UART_H

void mini_uart_init(void);
char mini_uart_getc(void);
void mini_uart_putc(char c);
void mini_uart_puts(const char *s);
void mini_uart_putln(const char *s);

#endif
