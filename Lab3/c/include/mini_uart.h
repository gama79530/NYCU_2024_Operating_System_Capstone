#ifndef LAB3_C_MINI_UART_H
#define LAB3_C_MINI_UART_H

/* Initialize GPIO pins and Mini UART registers for polling I/O. */
void mini_uart_init(void);

/* Read one character from Mini UART, converting carriage return to newline. */
char mini_uart_getc(void);

/* Write one character to Mini UART, expanding newline to CRLF. */
void mini_uart_putc(char c);

/* Write a null-terminated string to Mini UART. */
void mini_uart_puts(const char *s);

/* Write a string followed by a newline to Mini UART. */
void mini_uart_putln(const char *s);

#endif
