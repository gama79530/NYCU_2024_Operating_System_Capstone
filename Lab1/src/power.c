#include "power.h"
#include "gpio.h"
#include "mailbox.h"
#include "delays.h"

#define PM_BASE         MMIO_BASE + 0x00100000
#define PM_RSTC         ((volatile unsigned int*)(PM_BASE + 0x0000001c))
#define PM_RSTS         ((volatile unsigned int*)(PM_BASE + 0x00000020))
#define PM_WDOG         ((volatile unsigned int*)(PM_BASE + 0x00000024))
#define PM_WDOG_MAGIC   0x5a000000
#define PM_RSTC_FULLRST 0x00000020

void power_off(){
    unsigned long r;

    // power off devices one by one
    for(r = 0; r < 16; r++){
        mailbox[0] = 8 * 4;
        mailbox[1] = REQUEST_CODE;
        mailbox[2] = POWER_PFF; // set power state
        mailbox[3] = 8;
        mailbox[4] = 8;
        mailbox[5] = (unsigned int)r;   // device id
        mailbox[6] = 0;                 // bit 0: off, bit 1: no wait
        mailbox[7] = END_TAG;
        mailbox_call(8);
    }

    // power off gpio pins (but not VCC pins)
    *GPFSEL0 = 0; 
    *GPFSEL1 = 0; 
    *GPFSEL2 = 0; 
    *GPFSEL3 = 0; 
    *GPFSEL4 = 0; 
    *GPFSEL5 = 0;
    *GPPUD = 0;
    wait_cycles(150);
    *GPPUDCLK0 = 0xffffffff; 
    *GPPUDCLK1 = 0xffffffff;
    wait_cycles(150);
    *GPPUDCLK0 = 0; *GPPUDCLK1 = 0;        // flush GPIO setup

    // power off the SoC (GPU + CPU)
    r = *PM_RSTS; 
    r &= ~0xfffffaaa;
    r |= 0x555;    // partition 63 used to indicate halt
    *PM_RSTS = PM_WDOG_MAGIC | r;
    *PM_WDOG = PM_WDOG_MAGIC | 10;
    *PM_RSTC = PM_WDOG_MAGIC | PM_RSTC_FULLRST;
}

void reset(){
    unsigned int r;
    // trigger a restart by instructing the GPU to boot from partition 0
    r = *PM_RSTS; 
    r &= 0x555;
    *PM_RSTS = PM_WDOG_MAGIC | r;   // boot from partition 0
    *PM_RSTC = PM_WDOG_MAGIC | PM_RSTC_FULLRST;
    *PM_WDOG = PM_WDOG_MAGIC | 1000;
}

