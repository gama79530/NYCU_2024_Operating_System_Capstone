#include "types.h"
#include "mini_uart.h"
#include "dtb.h"
#include "cpio.h"
#include "exception.h"
#include "shell.h"
#include "memory.h"

int kernel_service_init(uint64_t x0){
    uart_init();
    set_dtb_ptr((void*)x0);
    if((cpio_init())) return -1;
    if(memory_system_init()) return -1;
    enable_interrupt_all();
    uart_enable_irqs_1();
    set_async_flag(true);
    uart_putln("");

    return 0;
}

void main(uint64_t x0){
    if(kernel_service_init(x0)) return;
    shell();
}