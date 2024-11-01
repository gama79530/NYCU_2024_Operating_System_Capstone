#include "types.h"
#include "mini_uart.h"
#include "printf.h"
#include "exception.h"
#include "dtb.h"
#include "cpio.h"
#include "config.h"
#include "shell.h"
#include "frame.h"
#include "memory.h"
#include "sched.h"

extern char kernel_begin;
extern char kernel_end;
extern char startup_heap_base;
extern char startup_heap_boundary;

int kernel_service_init(uint64_t x0){
    uart_init();
    enable_interrupt_all();
    uart_enable_irqs_1();
    init_printf(0, putc);
    if(dtb_init((void*)x0)) return -1;
    if(ramdisk_init()) return -1;
    /* build buddy system */
    if(buddy_sys_init())    return -1;
    buddy_sys_preserve_memory(
        (void*)SPIN_TABLE_BASE, 
        (void*)SPIN_TABLE_BOUNDARY, 
        "Preserve spin tables for multicore boot"
    );
    buddy_sys_preserve_memory((void*)&kernel_begin, (void*)&kernel_end, "Preserve kernel image");
    buddy_sys_preserve_memory(get_cpio_base(), get_cpio_boundary(), "Preserve ram disk");
    buddy_sys_preserve_memory(get_dtb_ptr(), get_dtb_ptr()+get_dtb_size(), "Preserve device tree");
    buddy_sys_preserve_memory((void*)&startup_heap_base, (void*)&startup_heap_boundary, "Preserve startup heap");
    buddy_sys_preserve_memory((void*)&kernel_begin - (1 << 10), (void*)&kernel_begin, "Preserve 1 kb for kernel stack");
    buddy_sys_build();

    /* build memory system */
    if(memory_sys_init())   return -1;

    time_sharing_enable();

    printf("\n");

    return 0;
}

void kernel_main(uint64_t x0){
    if(kernel_service_init(x0)) return;
    shell();
}