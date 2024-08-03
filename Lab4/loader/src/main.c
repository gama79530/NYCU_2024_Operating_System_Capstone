#include "mini_uart.h"
#include "shell.h"
#include "types.h"

uint64_t kernel_size;
uint64_t dtb_ptr;

void main(uint64_t x0){
    kernel_size = 0;
    dtb_ptr = x0;

    uart_init();
    shell();
}