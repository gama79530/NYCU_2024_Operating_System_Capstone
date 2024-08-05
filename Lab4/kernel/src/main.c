#include "types.h"
#include "mini_uart.h"
#include "dtb.h"
#include "cpio.h"
#include "exception.h"
#include "shell.h"

void kernel_service_init(uint64_t x0){
    uart_init();
    set_dtb_ptr((void*)x0);
    cpio_init();
    enable_interrupt_all();
    uart_enable_irqs_1();
}

void main(void){
    shell();
}