#include "common.h"
#include "mini_uart.h"
#include "shell.h"
#include "util.h"

uint32_t kernel_size = 0;
uint64_t dtb_ptr = 0;

void main(uint64_t x0)
{
    kernel_size = 0;
    dtb_ptr = x0;
    uart_init();
    shell();
}