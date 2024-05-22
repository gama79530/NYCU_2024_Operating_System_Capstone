#define MAILBOX_BASE    MMIO_BASE + 0xB880

#define MAILBOX_READ    ((volatile unsigned int*)(MAILBOX_BASE))
#define MAILBOX_STATUS  ((volatile unsigned int*)(MAILBOX_BASE + 0x18))
#define MAILBOX_WRITE   ((volatile unsigned int*)(MAILBOX_BASE + 0x20))

#define MAILBOX_EMPTY   0x40000000
#define MAILBOX_FULL    0x80000000

#include "gpio.h"
#include "mailbox.h"

/* mailbox message buffer */
volatile unsigned int __attribute__((aligned(16))) mailbox[36];

int mailbox_call(unsigned char channel){
    // step 1: Combine the message address (upper 28 bits) with channel number (lower 4 bits)
    unsigned int reg = ((unsigned int)((unsigned long)mailbox & ~0xF) | (channel & 0xF));
    // step 2: Check if Mailbox 0 status register’s full flag is set.
    do{ 
        asm volatile("nop");
    }while(*MAILBOX_STATUS & MAILBOX_FULL);
    // step 3: If not, then you can write to Mailbox 1 Read/Write register.
    *MAILBOX_WRITE = reg;
    while(1){
        // step 4: Check if Mailbox 0 status register’s empty flag is set.
        do{
            asm volatile("nop");
        }while(*MAILBOX_STATUS & MAILBOX_EMPTY);
        // step 5: If not, then you can read from Mailbox 0 Read/Write register.
        if(reg == *MAILBOX_READ){
            // step 6: Check if the value is the same as you wrote in step 1.
            return mailbox[1] != REQUEST_SUCCEED;
        }
    }
    
    return 1;
}