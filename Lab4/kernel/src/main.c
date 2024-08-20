#include "types.h"
#include "mini_uart.h"
#include "dtb.h"
#include "cpio.h"
#include "frame.h"
#include "exception.h"
#include "shell.h"

int kernel_service_init(uint64_t x0){
    int ret = 0;

    uart_init();

    set_dtb_ptr((void*)x0);
    if((ret = cpio_init())) return ret;
    if((ret = buddy_system_init())) return ret;
    enable_interrupt_all();
    uart_enable_irqs_1();

    return ret;
}

void main(uint64_t x0){
    if(kernel_service_init(x0)){
        return;
    }

    shell();
}