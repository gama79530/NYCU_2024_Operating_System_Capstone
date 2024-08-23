#include "types.h"
#include "mini_uart.h"
#include "dtb.h"
#include "cpio.h"
#include "exception.h"
#include "shell.h"

int kernel_service_init(uint64_t x0){
    int ret = 0;

    uart_init();

    set_dtb_ptr((void*)x0);
    if((ret = cpio_init())) return ret;
/* debug */
#include "frame.h"
#include "memory.h"
void *metadata = buddy_system_init(
    (void*)0x00000000,
    (void*)0x3C000000,
    12,
    16,
    startup_memory_alloc,
    startup_memory_preserve
);
buddy_show_layout(metadata);
/* debug */
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