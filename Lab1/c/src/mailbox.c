#include "mailbox.h"
#include "common.h"
#include "peripheral.h"

static volatile uint32_t __attribute__((aligned(16))) default_buffer[MAILBOX_BUFFER_SIZE];

const char *mailbox_err_msg[] = {
    "Mailbox response mismatch",
    "Mailbox exchange timeout",
};

volatile uint32_t *get_default_buffer(void)
{
    return default_buffer;
}

int mailbox_exchange(uint8_t channel, volatile uint32_t *buffer)
{
    if (buffer == NULL) {
        buffer = default_buffer;
    }
    // Encode mailbox address (upper 28 bits) and channel number (lower 4 bits)
    uint32_t reg = (uint32_t) (((uint64_t) buffer & ~CHANNEL_MASK) | (channel & CHANNEL_MASK));
    // Wait until mailbox 0 is not full
    while (get32(MBOX_STATUS) & MBOX_FULL)
        ;
    // Write message to mailbox read/write register
    put32(MBOX_WRITE, reg);

    // Wait for response until timeout
    for (uint64_t i = 0; i < TIMEOUT_MAX; i++) {
        // Read from Mailbox 0 Read/Write register when mailbox is not empty & address match
        if (!(get32(MBOX_STATUS) & MBOX_EMPTY) && reg == get32(MBOX_READ)) {
            return (buffer[1] == MBOX_RESPONSE ? MAILBOX_SUCCESS : MAILBOX_ERR_RESPONSE);
        }
    }
    return MAILBOX_ERR_TIMEOUT;
}

const char *err_code_to_str(int err_code)
{
    if (err_code >= 0) {
        return NULL;
    }
    return mailbox_err_msg[~err_code];
}
