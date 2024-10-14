#ifndef __MINI_UART_H__
#define __MINI_UART_H__
#include "types.h"

void uart_init(void);

/* for printf */
void putc(void *p, char c);

#if USE_ASYNC_IO == 0
    #define uart_getb           uart_poll_getb
    #define uart_getc           uart_poll_getc
    #define uart_putc           uart_poll_putc
    #define uart_puts           uart_poll_puts
    #define uart_putln          uart_poll_putln
    #define uart_put_mutiln     uart_poll_put_mutiln
#else
    #define uart_getb           uart_async_getb
    #define uart_getc           uart_async_getc
    #define uart_putc           uart_async_putc
    #define uart_puts           uart_async_puts
    #define uart_putln          uart_async_putln
    #define uart_put_mutiln     uart_async_put_mutiln
#endif

/* polling io */
char uart_poll_getb(void);
char uart_poll_getc(void);
void uart_poll_putc(char c);
void uart_poll_puts(const char *s);
void uart_poll_putln(const char *s);
void uart_poll_put_mutiln(const char *s, uint32_t len);

/* async io*/
char uart_async_getb(void);
char uart_async_getc(void);
void uart_async_putc(char c);
void uart_async_puts(const char *s);
void uart_async_putln(const char *s);
void uart_async_put_mutiln(const char *s, uint32_t len);

/* buffer size should be power of 2 */
#define RX_BUFFER_SIZE  (1 << 6)
#define TX_BUFFER_SIZE  (1 << 6)

bool uart_rx_buffer_is_empty();
bool uart_rx_buffer_is_full();
bool uart_tx_buffer_is_empty();
bool uart_tx_buffer_is_full();

char uart_get_from_rx_buffer(void);
void uart_put_to_rx_buffer(char c);
char uart_get_from_tx_buffer();
void uart_put_to_tx_buffer(char c);

void uart_enable_irqs_1(void);
void uart_disable_irqs_1(void);
void uart_rx_set(void);
void uart_rx_clr(void);
void uart_tx_set(void);
void uart_tx_clr(void);
void handler_uart_rx(void);
void handler_uart_tx(void);

#endif