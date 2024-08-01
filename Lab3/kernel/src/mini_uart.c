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

void uart_init(void){
    register unsigned int selector;
    
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

void enable_irqs_1(void){
    put32(ENABLE_IRQs_1, IRQ_AUX_INT);
}

void disable_irqs_1(void){
    put32(DISABLE_IRQs_1, IRQ_AUX_INT);
}

void interrupt_rx_set(void){
    uint32_t reg = get32(AUX_MU_IER);
    reg |= AUX_ENABLE_RX;
    put32(AUX_MU_IER, reg);
}

void interrupt_rx_clr(void){
    uint32_t reg = get32(AUX_MU_IER);
    reg &= ~AUX_ENABLE_RX;
    put32(AUX_MU_IER, reg);
}

void interrupt_tx_set(void){
    uint32_t reg = get32(AUX_MU_IER);
    reg |= AUX_ENABLE_TX;
    put32(AUX_MU_IER, reg);
}

void interrupt_tx_clr(void){
    uint32_t reg = get32(AUX_MU_IER);
    reg &= ~AUX_ENABLE_TX;
    put32(AUX_MU_IER, reg);
}

char uart_getb(void){
    char c;
    while(!(get32(AUX_MU_LSR) & 0x01));     // wait until something is in the buffer
    c = (char)(get32(AUX_MU_IO) & 0xFF);    // read the character from the buffer
    return c;
}

char uart_getc(void){
    char c = uart_getb();
    // convert carriage return to newline
    if(c == '\r'){
        c = '\n';
    }
    return c;
}

void uart_putc(char c){
    while(!(get32(AUX_MU_LSR) & 0x20));     // wait until we can send
    put32(AUX_MU_IO, c);                    // write the character to the buffer
}

void uart_puts(const char *s){
    while(*s){
        uart_putc(*s++);
    }
}

void uart_putln(const char *s){
    uart_puts(s);
    uart_puts("\r\n");
}

void uart_put_mutiln(const char *s, uint32_t len){
    while(*s && len){
        // convert newline to carriage return + newline
        if(*s == '\n'){ 
            uart_putc('\r');
        }
        uart_putc(*s++);
        if(len > 0){
            len--;
        }
    }
    if(!len){
        uart_putln("");
    }
}

/* async io */
char uart_async_getb(void){
    interrupt_rx_set();
    char c;
    while(RX_BUFFER_EMPTY){
        asm volatile("nop");
    }
    disable_irqs_1();   // mask aux irq while we retrive data from buffer
    c = get_from_rx_buffer();
    enable_irqs_1();
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
        interrupt_tx_set();
        while(!TX_BUFFER_EMPTY){
            asm volatile("nop");
        }
    }
    put_to_tx_buffer(c);
    interrupt_tx_set();
}

int rx_is_empty(){
    return RX_BUFFER_EMPTY;
}

int rx_is_full(){
    return RX_BUFFER_FULL;
}

char get_from_rx_buffer(void){
    char c = rx_buffer[rx_head++];
    rx_head &= (RX_BUFFER_SIZE - 1);    // equivalent to "rx_head %= RX_BUFFER_SIZE;" while TX_BUFFER_SIZE is power of 2.
    return c;
}

void put_to_rx_buffer(char c){
    rx_buffer[rx_tail++] = c;
    rx_tail &= (RX_BUFFER_SIZE - 1);    // equivalent to "rx_tail %= RX_BUFFER_SIZE;" while RX_BUFFER_SIZE is power of 2.
}

int tx_is_empty(){
    return TX_BUFFER_EMPTY;
}

int tx_is_full(){
    return TX_BUFFER_FULL;
}

char get_from_tx_buffer(){
    char c = tx_buffer[tx_head++];
    tx_head &= (TX_BUFFER_SIZE - 1);    // equivalent to "tx_head %= TX_BUFFER_SIZE;" while TX_BUFFER_SIZE is power of 2.
    return c;
}

void put_to_tx_buffer(char c){
    tx_buffer[tx_tail++] = c;
    tx_tail &= (TX_BUFFER_SIZE - 1);    // equivalent to "tx_tail %= TX_BUFFER_SIZE;" while RX_BUFFER_SIZE is power of 2.
}

void handler_mini_uart_rx(void){
    disable_irqs_1();
    uart_putln("Receiver holds valid byte");
    put_to_rx_buffer(uart_getb());
    interrupt_rx_clr();
    enable_irqs_1();
}

void handler_mini_uart_tx(void){
    disable_irqs_1();
    uart_putln("Transmit holding register empty");
    while(!tx_is_empty()){
        uart_putc(get_from_tx_buffer());
    }
    // enable_irqs_1();
}