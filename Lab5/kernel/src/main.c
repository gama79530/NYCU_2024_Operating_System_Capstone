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
#include "thread.h"
#include "timer.h"

extern char kernel_begin;
extern char kernel_end;
extern char startup_heap_base;
extern char startup_heap_boundary;

int kernel_service_init(uint64_t x0){
    uart_init();
    enable_all_exception();
#if USE_ASYNC_IO != 0
    uart_enable_irqs_1();
#endif
    init_printf(0, putc);
    if(dtb_init((void*)x0)){
        return -1;
    }
    if(ramdisk_init()){
        return -1;
    }
    /* build buddy system */
    if(buddy_sys_init()){
        return -1;
    }
    if(buddy_sys_preserve_memory((void*)SPIN_TABLE_BASE, (void*)SPIN_TABLE_BOUNDARY, 
       "Preserve spin tables for multicore boot"))                      return -1;
    if(buddy_sys_preserve_memory((void*)&kernel_begin, (void*)&kernel_end, 
       "Preserve kernel image"))                                        return -1;
    if(buddy_sys_preserve_memory(get_cpio_base(), get_cpio_boundary(), 
       "Preserve ram disk"))                                            return -1;
    if(buddy_sys_preserve_memory(get_dtb_ptr(), get_dtb_ptr() + get_dtb_size(), 
       "Preserve device tree"))                                         return -1;
    if(buddy_sys_preserve_memory((void*)&startup_heap_base, (void*)&startup_heap_boundary, 
       "Preserve startup heap"))                                        return -1;
    if(buddy_sys_preserve_memory((void*)&kernel_begin - (1 << 10), (void*)&kernel_begin, 
       "Preserve 1 kb for kernel stack"))                               return -1;
    if(buddy_sys_build())                                               return -1;

    /* build memory system */
    if(memory_sys_init()){
        return -1;
    }

    /* initialize timer */
    if(timer_init()){
        return -1;
    }

    /* initialize time-sharing */
    if(scheduling_init()){
        return -1;
    }

    printf("\n");

    return 0;
}

void kernel_main(uint64_t x0){
    if(kernel_service_init(x0)){
        return;
    }
    thread_create(0, (task_routine_t)shell, NULL);
    idle();
}
