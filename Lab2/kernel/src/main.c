#include "mini_uart.h"
#include "shell.h"
#include "string.h"

unsigned int kernel_size;
unsigned long dtb_ptr;

void main(unsigned long x0){
    kernel_size = 0;
    dtb_ptr = x0;

    uart_init();
    shell();
}