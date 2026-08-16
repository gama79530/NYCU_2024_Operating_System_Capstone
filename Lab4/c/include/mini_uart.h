#ifndef LAB4_C_MINI_UART_H
#define LAB4_C_MINI_UART_H

#include "types.h"

/* Initialize GPIO pins and Mini UART registers in polling mode. */
void mini_uart_init(void);

/* Enable buffered RX/TX I/O and route Mini UART interrupts to the CPU. */
void mini_uart_enable_async(void);

/* Return whether the BCM interrupt controller reports a Mini UART IRQ. */
bool mini_uart_irq_pending(void);

/* Mask pending UART sources and enqueue their deferred RX/TX work. */
void mini_uart_handle_irq(void);

/* Read one character, using the RX buffer after async I/O is enabled. */
char mini_uart_getc(void);

/* Write one character, using the TX buffer after async I/O is enabled. */
void mini_uart_putc(char c);

/* Write a null-terminated string to Mini UART. */
void mini_uart_puts(const char *s);

/* Write a string followed by a newline to Mini UART. */
void mini_uart_putln(const char *s);

#endif
