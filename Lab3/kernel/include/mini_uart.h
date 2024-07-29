#ifndef MINI_UART_H
#define MINI_UART_H
#include "types.h"

/* buffer size should be power of 2 */
#define RX_BUFFER_SIZE  (1 << 6)
#define TX_BUFFER_SIZE  (1 << 6)

/* setting */
void uart_init(void);
void enable_irqs_1(void);
void disable_irqs_1(void);
void interrupt_rx_set(void);
void interrupt_rx_clr(void);
void interrupt_tx_set(void);
void interrupt_tx_clr(void);

/* polling io */
char uart_getb(void);
char uart_getc(void);
void uart_putc(char c);

void uart_puts(const char *s);
void uart_putln(const char *s);
void uart_put_mutiln(const char *s, uint32_t len);

/* async io*/
int rx_is_empty();
int rx_is_full();
char get_from_rx(void);
void put_to_rx(char c);

int tx_is_empty();
int tx_is_full();
char get_from_tx();
void put_to_tx(char c);

char uart_async_getb(void);
char uart_async_getc(void);
void uart_async_putc(char c);


#endif