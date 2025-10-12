#include "mini_uart.hpp"
#include "common.hpp"
#include "peripheral.hpp"

MiniUart::MiniUart()
{
    uint32_t selector;
    /* map UART1 to GPIO pins */
    selector = util::get32(GPFSEL1);
    selector &= ~((7 << 12) | (7 << 15));  // clean gpio14, gpio15
    selector |= (2 << 12) | (2 << 15);     // set alternative function alt5 for gpio14 and gpio15
    util::put32(GPFSEL1, selector);

    /* GPIO pull-up/down */
    util::put32(GPPUD, 0);  // Write to GPPUD to set the required control signal
    util::wait_cycles(
        150);  // Wait 150 cycles – this provides the required set-up time for the control signal

    util::put32(GPPUDCLK0,
                (1 << 14) | (1 << 15));  // Write to GPPUDCLK0/1 to clock the control signal
                                         // into the GPIO pads you wish to modify
    util::wait_cycles(
        150);  // Wait 150 cycles – this provides the required hold time for the control signal
    util::put32(GPPUDCLK0, 0);  // Write to GPPUD to remove the control signal

    /* initialize UART */
    util::put32(AUX_ENABLES, 1);  // Enable mini uart (this also enables access to its registers)
    util::put32(AUX_MU_CNTL,
                0);  // Disable auto flow control and disable receiver and transmitter (for now)
    util::put32(AUX_MU_LCR, 3);     // Enable 8 bit mode
    util::put32(AUX_MU_MCR, 0);     // Set RTS line to be always high
    util::put32(AUX_MU_IER, 0);     // Disable receive and transmit interrupts
    util::put32(AUX_MU_IIR, 0xc6);  // disable interrupts
    util::put32(AUX_MU_BAUD, 270);  // Set baud rate to 115200
    util::put32(AUX_MU_CNTL, 3);    // Finally, enable transmitter and receiver
}

char MiniUart::getb()
{
    char c;
    while (!(util::get32(AUX_MU_LSR) & 0x01))
        ;                                        // wait until something is in the buffer
    c = (char) (util::get32(AUX_MU_IO) & 0xFF);  // read the character from the buffer
    return c;
}

char MiniUart::getc()
{
    char c = this->getb();
    // convert carriage return to newline
    return c == '\r' ? '\n' : c;
}

void MiniUart::putc(char c)
{
    while (!(util::get32(AUX_MU_LSR) & 0x20))
        ;                       // wait until we can send
    util::put32(AUX_MU_IO, c);  // write the character to the buffer
}

void MiniUart::puts(const char *s)
{
    while (*s) {
        // convert newline to carriage return + newline
        if (*s == '\n') {
            this->putc('\r');
        }
        this->putc(*s++);
    }
}

void MiniUart::putln(const char *s)
{
    this->puts(s);
    this->puts("\n");
}
