#include "mini_uart.h"
#include "util.h"
#include "peripheral.h"

#define RX_BUFFER_EMPTY    (rx_head == rx_tail)
#define RX_BUFFER_FULL     (rx_head == (rx_tail + 1) % RX_BUFFER_SIZE)
#define TX_BUFFER_EMPTY    (tx_head == tx_tail)
#define TX_BUFFER_FULL     (tx_head == (tx_tail + 1) % TX_BUFFER_SIZE)

static char rx_buffer[RX_BUFFER_SIZE] = {0};
static char tx_buffer[TX_BUFFER_SIZE] = {0};
static uint32_t rx_head = 0;
static uint32_t rx_tail = 0;
static uint32_t tx_head = 0;
static uint32_t tx_tail = 0;
static bool _is_irq_set = false;

void uart_init(void){
    register uint32_t selector;
    
    /* map UART1 to GPIO pins */
    selector = get32(GPFSEL1);
    selector &= ~((7 << 12) | (7 << 15));       // clean gpio14, gpio15
    selector |= (2 << 12) | (2 << 15);          // set alternative function alt5 for gpio14 and gpio15
    put32(GPFSEL1, selector);
    
    /* GPIO pull-up/down */
    put32(GPPUD, 0);                            // Write to GPPUD to set the required control signal
    wait_cycles(150);                           // Wait 150 cycles – this provides the required set-up time for the control signal

    put32(GPPUDCLK0, (1 << 14) | (1 << 15));    // Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to modify
    wait_cycles(150);                           // Wait 150 cycles – this provides the required hold time for the control signal
    put32(GPPUDCLK0, 0);                        // Write to GPPUD to remove the control signal

    /* initialize UART */
    put32(AUX_ENABLES, 1);                      // Enable mini uart (this also enables access to its registers)
    put32(AUX_MU_CNTL, 0);                      // Disable auto flow control and disable receiver and transmitter (for now)
    put32(AUX_MU_LCR, 3);                       // Enable 8 bit mode
    put32(AUX_MU_MCR, 0);                       // Set RTS line to be always high
    put32(AUX_MU_IER, 0);                       // Disable receive and transmit interrupts
    put32(AUX_MU_IIR, 0xc6);                    // disable interrupts
    put32(AUX_MU_BAUD, 270);                    // Set baud rate to 115200
    put32(AUX_MU_CNTL, 3);                      // Finally, enable transmitter and receiver
}

void uart_enable_irqs_1(void){
    put32(ENABLE_IRQs_1, IRQ_AUX_INT);
    _is_irq_set = true;
}

void uart_disable_irqs_1(void){
    put32(DISABLE_IRQs_1, IRQ_AUX_INT);
    _is_irq_set = false;
}

void uart_rx_set(void){
    uint32_t reg = get32(AUX_MU_IER);
    reg |= AUX_ENABLE_RX;
    put32(AUX_MU_IER, reg);
}

void uart_rx_clr(void){
    uint32_t reg = get32(AUX_MU_IER);
    reg &= ~AUX_ENABLE_RX;
    put32(AUX_MU_IER, reg);
}

void uart_tx_set(void){
    uint32_t reg = get32(AUX_MU_IER);
    reg |= AUX_ENABLE_TX;
    put32(AUX_MU_IER, reg);
}

void uart_tx_clr(void){
    uint32_t reg = get32(AUX_MU_IER);
    reg &= ~AUX_ENABLE_TX;
    put32(AUX_MU_IER, reg);
}

char uart_poll_getb(void){
    char c;
    while(!(get32(AUX_MU_LSR) & 0x01));     // wait until something is in the buffer
    c = (char)(get32(AUX_MU_IO) & 0xFF);    // read the character from the buffer
    return c;
}

char uart_poll_getc(void){
    char c = uart_poll_getb();
    // convert carriage return to newline
    if(c == '\r'){
        c = '\n';
    }
    return c;
}

void uart_poll_putc(char c){
    while(!(get32(AUX_MU_LSR) & 0x20));     // wait until we can send
    put32(AUX_MU_IO, c);                    // write the character to the buffer
}

void uart_poll_puts(const char *s){
    while(*s){
        uart_poll_putc(*s++);
    }
}

void uart_poll_putln(const char *s){
    uart_poll_puts(s);
    uart_poll_puts("\r\n");
}

void uart_poll_put_mutiln(const char *s, uint32_t len){
    while(*s && len){
        // convert newline to carriage return + newline
        if(*s == '\n'){ 
            uart_poll_putc('\r');
        }
        uart_poll_putc(*s++);
        if(len > 0){
            len--;
        }
    }
    if(!len){
        uart_poll_puts("\r\n");
    }
}

/* async io */
char uart_async_getb(void){
    uart_rx_set();
    char c;
    while(RX_BUFFER_EMPTY){
        asm volatile("nop");
    }
    uart_disable_irqs_1();   // mask aux irq while we retrive data from buffer
    c = uart_get_from_rx_buffer();
    uart_enable_irqs_1();
    return c;
}

char uart_async_getc(void){
    char c = uart_async_getb();
    // convert carriage return to newline
    if(c == '\r'){
        c = '\n';
    }
    return c;
}

void uart_async_putc(char c){
    if(TX_BUFFER_FULL){
        uart_tx_set();
        while(!TX_BUFFER_EMPTY){
            asm volatile("nop");
        }
    }
    uart_put_to_tx_buffer(c);
    uart_tx_set();
}

void uart_async_puts(const char *s){
    while(*s){
        uart_async_putc(*s++);
    }
}

void uart_async_putln(const char *s){
    uart_async_puts(s);
    uart_async_puts("\r\n");
}

void uart_async_put_mutiln(const char *s, uint32_t len){
    while(*s && len){
        // convert newline to carriage return + newline
        if(*s == '\n'){ 
            uart_async_putc('\r');
        }
        uart_async_putc(*s++);
        if(len > 0){
            len--;
        }
    }
    if(!len){
        uart_async_puts("\r\n");
    }
}

int uart_rx_is_empty(){
    return RX_BUFFER_EMPTY;
}

int uart_rx_is_full(){
    return RX_BUFFER_FULL;
}

char uart_get_from_rx_buffer(void){
    char c = rx_buffer[rx_head++];
    rx_head &= (RX_BUFFER_SIZE - 1);    // equivalent to "rx_head %= RX_BUFFER_SIZE;" while TX_BUFFER_SIZE is power of 2.
    return c;
}

void uart_put_to_rx_buffer(char c){
    rx_buffer[rx_tail++] = c;
    rx_tail &= (RX_BUFFER_SIZE - 1);    // equivalent to "rx_tail %= RX_BUFFER_SIZE;" while RX_BUFFER_SIZE is power of 2.
}

int uart_tx_is_empty(){
    return TX_BUFFER_EMPTY;
}

int uart_tx_is_full(){
    return TX_BUFFER_FULL;
}

char uart_get_from_tx_buffer(){
    char c = tx_buffer[tx_head++];
    tx_head &= (TX_BUFFER_SIZE - 1);    // equivalent to "tx_head %= TX_BUFFER_SIZE;" while TX_BUFFER_SIZE is power of 2.
    return c;
}

void uart_put_to_tx_buffer(char c){
    tx_buffer[tx_tail++] = c;
    tx_tail &= (TX_BUFFER_SIZE - 1);    // equivalent to "tx_tail %= TX_BUFFER_SIZE;" while RX_BUFFER_SIZE is power of 2.
}

void handler_uart_rx(void){
    uart_disable_irqs_1();
    uart_put_to_rx_buffer(uart_poll_getb());
    uart_enable_irqs_1();
}

void handler_uart_tx(void){
    uart_disable_irqs_1();
    while(!uart_tx_is_empty()){
        uart_poll_putc(uart_get_from_tx_buffer());
    }
    uart_enable_irqs_1();
}

bool is_irq_set(){
    return _is_irq_set;
}
