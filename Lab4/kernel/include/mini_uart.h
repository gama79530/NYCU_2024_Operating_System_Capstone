#ifndef MINI_UART_H
#define MINI_UART_H
#include "types.h"

// make synonym
#define uart_getb       (is_irq_1_enable() ? uart_async_getb : uart_poll_getb)
#define uart_getc       (is_irq_1_enable() ? uart_async_getc : uart_poll_getc)
#define uart_putc       (is_irq_1_enable() ? uart_async_putc : uart_poll_putc)
#define uart_puts       (is_irq_1_enable() ? uart_async_puts : uart_poll_puts)
#define uart_putln      (is_irq_1_enable() ? uart_async_putln : uart_poll_putln)
#define uart_put_mutiln (is_irq_1_enable() ? uart_async_put_mutiln : uart_poll_put_mutiln)

// #define uart_getb       uart_poll_getb
// #define uart_getc       uart_poll_getc
// #define uart_putc       uart_poll_putc
// #define uart_puts       uart_poll_puts
// #define uart_putln      uart_poll_putln
// #define uart_put_mutiln uart_poll_put_mutiln

/* buffer size should be power of 2 */
#define RX_BUFFER_SIZE  (1 << 6)
#define TX_BUFFER_SIZE  (1 << 6)

/* setting */
void uart_init(void);
void uart_enable_irqs_1(void);
void uart_disable_irqs_1(void);
bool is_irq_1_enable();
void uart_rx_set(void);
void uart_rx_clr(void);
void uart_tx_set(void);
void uart_tx_clr(void);

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

int uart_rx_is_empty();
int uart_rx_is_full();
char uart_get_from_rx_buffer(void);
void uart_put_to_rx_buffer(char c);

int uart_tx_is_empty();
int uart_tx_is_full();
char uart_get_from_tx_buffer();
void uart_put_to_tx_buffer(char c);

void handler_uart_rx(void);
void handler_uart_tx(void);

#endif