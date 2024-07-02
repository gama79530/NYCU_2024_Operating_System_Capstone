#include "mini_uart.h"
#include "shell.h"
#include "string.h"
#include "dtb.h"
#include "cpio.h"

void main(unsigned long x0){
    set_dtb_ptr((void*)x0);
    uart_init();
    cpio_init();
    shell();
}