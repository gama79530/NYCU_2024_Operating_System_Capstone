#include "mailbox.h"
#include "peripheral.h"
#include "util.h"

/* mailbox message buffer */
volatile uint32_t __attribute__((aligned(16))) mbox[36];

int mailbox_call(uint8_t channel){
    // step 1: Combine the message address (upper 28 bits) with channel number (lower 4 bits)
    uint32_t r = (uint32_t)(((uint64_t)mbox & ~0xF) | (channel & 0xF));
    // step 2: Check if Mailbox 0 status register’s full flag is set.
    while(get32(MBOX_STATUS) & MBOX_FULL);
    // step 3: If not, then you can write to Mailbox 1 Read/Write register.
    put32(MBOX_WRITE, r);

    while(1){
        // step 4: Check if Mailbox 0 status register’s empty flag is set.
        while(get32(MBOX_STATUS) & MBOX_EMPTY);
        // step 5: If not, then you can read from Mailbox 0 Read/Write register.
        if(r == get32(MBOX_READ)){
            // step 6: Check if the value is the same as you wrote in step 1.
            return mbox[1] == MBOX_RESPONSE;
        }
    }

    return 0;
}