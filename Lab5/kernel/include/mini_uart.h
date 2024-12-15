#ifndef __MINI_UART_H__
#define __MINI_UART_H__
#include "types.h"

void uart_init(void);

/* for printf */
void putc(void *p, char c);

/* polling io */
char uart_poll_getb(void);
char uart_poll_getc(void);
void uart_poll_putc(char c);
void uart_poll_puts(const char *s);
void uart_poll_putln(const char *s);

/* async io*/
char uart_async_getb(void);
char uart_async_getc(void);
void uart_async_putc(char c);
void uart_async_puts(const char *s);
void uart_async_putln(const char *s);

void uart_enable_irqs_1(void);
void uart_disable_irqs_1(void);
void uart_rx_set(void);
void uart_rx_clr(void);
void uart_tx_set(void);
void uart_tx_clr(void);

bool uart_rx_buffer_is_empty();
bool uart_rx_buffer_is_full();
bool uart_tx_buffer_is_empty();
bool uart_tx_buffer_is_full();

char uart_get_from_rx_buffer(void);
void uart_put_to_rx_buffer(char c);
char uart_get_from_tx_buffer();
void uart_put_to_tx_buffer(char c);

void uart_irq_task_cb_rx(void);
void uart_irq_task_cb_tx(void);

#endif