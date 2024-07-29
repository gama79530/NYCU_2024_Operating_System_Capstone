#include "mini_uart.h"
#include "dtb.h"
#include "cpio.h"
#include "exception.h"
#include "shell.h"

void main(unsigned long x0){
    uart_init();
    set_dtb_ptr((void*)x0);
    cpio_init();
    enable_interrupt_all();
    shell();
}