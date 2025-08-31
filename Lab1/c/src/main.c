#include "common.h"
#include "mini_uart.h"
#include "printf.h"
#include "shell.h"

void kernel_service_init();
void putc(void *p, char c);

void main()
{
    kernel_service_init();
    shell();
}

void kernel_service_init(){
    uart_init();
    init_printf(NULL, putc);
}

void putc(void *p, char c){
    uart_putc(c);
}
