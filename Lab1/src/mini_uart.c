#include "gpio.h"
#include "mini_uart.h"

/* Auxilary mini UART registers */
#define AUX_ENABLE      ((volatile unsigned int*)(MMIO_BASE + 0x00215004))
#define AUX_MU_IO       ((volatile unsigned int*)(MMIO_BASE + 0x00215040))
#define AUX_MU_IER      ((volatile unsigned int*)(MMIO_BASE + 0x00215044))
#define AUX_MU_IIR      ((volatile unsigned int*)(MMIO_BASE + 0x00215048))
#define AUX_MU_LCR      ((volatile unsigned int*)(MMIO_BASE + 0x0021504C))
#define AUX_MU_MCR      ((volatile unsigned int*)(MMIO_BASE + 0x00215050))
#define AUX_MU_LSR      ((volatile unsigned int*)(MMIO_BASE + 0x00215054))
#define AUX_MU_MSR      ((volatile unsigned int*)(MMIO_BASE + 0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(MMIO_BASE + 0x0021505C))
#define AUX_MU_CNTL     ((volatile unsigned int*)(MMIO_BASE + 0x00215060))
#define AUX_MU_STAT     ((volatile unsigned int*)(MMIO_BASE + 0x00215064))
#define AUX_MU_BAUD     ((volatile unsigned int*)(MMIO_BASE + 0x00215068))

void uart_init(){
    register unsigned int selector;
    /* map UART1 to GPIO pins */
    selector = *GPFSEL1;
    selector &= ~((7 << 12) | (7 << 15));   // clean gpio14, gpio15
    selector |= (2 << 12) | (2 << 15);      // set alternative function alt5
    *GPFSEL1 = selector;
    /* GPIO pull-up/down */
    *GPPUD = 0;                             // Write to GPPUD to set the required control signal
    selector = 150;                         // Wait 150 cycles – this provides the required set-up time for the control signal
    while(selector--){ 
        asm volatile("nop");
    }
    *GPPUDCLK0 = (1 << 14) | (1 << 15);     // Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to modify
    selector = 150;                         // Wait 150 cycles – this provides the required hold time for the control signal
    while(selector--){ 
        asm volatile("nop");
    }
    *GPPUDCLK0 = 0;                         // Write to GPPUD to remove the control signal

    /* initialize UART */
    *AUX_ENABLE = 1;        // Enable mini uart (this also enables access to its registers)
    *AUX_MU_CNTL = 0;       // Disable auto flow control and disable receiver and transmitter (for now)
    *AUX_MU_IER = 0;        // Disable receive and transmit interrupts
    *AUX_MU_LCR = 3;        // Enable 8 bit mode
    *AUX_MU_MCR = 0;        // Set RTS line to be always high
    *AUX_MU_BAUD = 270;     // Set baud rate to 115200
    *AUX_MU_IIR = 0xc6;     // disable interrupts
    *AUX_MU_CNTL = 3;       // Finally, enable transmitter and receiver
}

void uart_putc(unsigned int c){
    /* wait until we can send */
    do{
        asm volatile("nop");
    }while(!(*AUX_MU_LSR & 0x20));
    /* write the character to the buffer */
    *AUX_MU_IO = c;
}

char uart_getc(void){
    char r;
    /* wait until something is in the buffer */
    do{
        asm volatile("nop");
    }while(!(*AUX_MU_LSR & 0x01));

    /* read it and return */
    r = (char)(*AUX_MU_IO);
    
    /* convert carriage return to newline */
    return (r == '\r' ? '\n' : r);
}

void uart_puts(char *s){
    while(*s){
        /* convert newline to carriage return + newline */
        if(*s == '\n'){
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

void uart_put_hex(unsigned int c){
    unsigned int hex;
    for (int i = 28; i >= 0; i -= 4){
        hex = (c >> i) & 0xF;
        // 0-9 => '0'-'9', 10-15 => 'A'-'F'
        hex += hex > 9 ? 0x37 : 0x30;
        uart_putc(hex);
    }
}