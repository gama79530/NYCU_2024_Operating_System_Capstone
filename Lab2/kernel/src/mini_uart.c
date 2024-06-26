#include "mini_uart.h"
#include "gpio.h"
#include "aux.h"
#include "delay.h"

void uart_init(void){
    register unsigned int selector;
    
    /* map UART1 to GPIO pins */
    selector = *GPFSEL1;
    selector &= ~((7 << 12) | (7 << 15));   // clean gpio14, gpio15
    selector |= (2 << 12) | (2 << 15);      // set alternative function alt5 for gpio14 and gpio15
    *GPFSEL1 = selector;
    
    /* GPIO pull-up/down */
    *GPPUD = 0;                             // Write to GPPUD to set the required control signal
    wait_cycles(150);                       // Wait 150 cycles – this provides the required set-up time for the control signal

    *GPPUDCLK0 = (1 << 14) | (1 << 15);     // Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to modify
    wait_cycles(150);                       // Wait 150 cycles – this provides the required hold time for the control signal
    *GPPUDCLK0 = 0;                         // Write to GPPUD to remove the control signal

    /* initialize UART */
    *AUX_ENABLES = 1;       // Enable mini uart (this also enables access to its registers)
    *AUX_MU_CNTL = 0;       // Disable auto flow control and disable receiver and transmitter (for now)
    *AUX_MU_LCR = 3;        // Enable 8 bit mode
    *AUX_MU_MCR = 0;        // Set RTS line to be always high
    *AUX_MU_IER = 0;        // Disable receive and transmit interrupts
    *AUX_MU_IIR = 0xc6;     // disable interrupts
    *AUX_MU_BAUD = 270;     // Set baud rate to 115200
    *AUX_MU_CNTL = 3;       // Finally, enable transmitter and receiver
}

char uart_getb(void){
    char c;
    while(!(*AUX_MU_LSR & 0x01));   // wait until something is in the buffer
    c = (char)(*AUX_MU_IO & 0xFF);  // read the character from the buffer
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
    while(!(*AUX_MU_LSR & 0x20));   // wait until we can send
    *AUX_MU_IO = c;                 // write the character to the buffer
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

void uart_put_mutiln(const char *s, int len){
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