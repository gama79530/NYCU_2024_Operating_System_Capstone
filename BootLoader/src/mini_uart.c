#include "mini_uart.h"

#include "peripheral.h"
#include "util.h"

void mini_uart_init(void)
{
    uint32_t selector;

    selector = get32(GPFSEL1);
    selector &= ~((7 << 12) | (7 << 15));
    selector |= (2 << 12) | (2 << 15);
    put32(GPFSEL1, selector);

    put32(GPPUD, 0);
    wait_cycles(150);
    put32(GPPUDCLK0, (1 << 14) | (1 << 15));
    wait_cycles(150);
    put32(GPPUDCLK0, 0);

    put32(AUX_ENABLES, 1);
    put32(AUX_MU_CNTL, 0);
    put32(AUX_MU_LCR, 3);
    put32(AUX_MU_MCR, 0);
    put32(AUX_MU_IER, 0);
    put32(AUX_MU_IIR, 0xc6);
    put32(AUX_MU_BAUD, 270);
    put32(AUX_MU_CNTL, 3);
}

uint8_t mini_uart_getb(void)
{
    while ((get32(AUX_MU_LSR) & 0x01) == 0) {
    }
    return (uint8_t)(get32(AUX_MU_IO) & 0xff);
}

char mini_uart_getc(void)
{
    char c = (char)mini_uart_getb();

    return c == '\r' ? '\n' : c;
}

void mini_uart_putc(char c)
{
    if (c == '\n') {
        mini_uart_putc('\r');
    }
    while ((get32(AUX_MU_LSR) & 0x20) == 0) {
    }
    put32(AUX_MU_IO, (uint32_t)c);
}

void mini_uart_puts(const char *s)
{
    while (*s != '\0') {
        mini_uart_putc(*s++);
    }
}

void mini_uart_putln(const char *s)
{
    mini_uart_puts(s);
    mini_uart_putc('\n');
}
