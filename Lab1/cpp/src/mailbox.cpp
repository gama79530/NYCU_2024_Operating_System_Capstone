#include "mailbox.hpp"
#include "common.hpp"
#include "peripheral.hpp"

const char *const Mailbox::errMessages[] = {
    "Mailbox response mismatch",
    "Mailbox exchange timeout",
};

Mailbox::Mailbox() {}

Mailbox::ReturnCode Mailbox::mailboxExchange(uint8_t channel, volatile uint32_t *buffer)
{
    // Encode mailbox address (upper 28 bits) and channel number (lower 4 bits)
    uint32_t reg = (uint32_t) (((uint64_t) buffer & ~CHANNEL_MASK) | (channel & CHANNEL_MASK));
    // Wait until mailbox 0 is not full
    while (util::get32(MBOX_STATUS) & MBOX_FULL)
        ;
    // Write message to mailbox read/write register
    util::put32(MBOX_WRITE, reg);

    // Wait for response until timeout
    for (uint64_t i = 0; i < TIMEOUT_MAX; i++) {
        // Read from Mailbox 0 Read/Write register when mailbox is not empty & address match
        if (!(util::get32(MBOX_STATUS) & MBOX_EMPTY) && reg == util::get32(MBOX_READ)) {
            return (buffer[1] == MBOX_RESPONSE ? MAILBOX_SUCCESS : MAILBOX_ERR_RESPONSE);
        }
    }
    return MAILBOX_ERR_TIMEOUT;
}

Mailbox::ReturnCode Mailbox::mailboxExchange(uint8_t channel)
{
    return mailboxExchange(channel, buffer);
}


const char *Mailbox::errorCodeToStr(int32_t errCode)
{
    if (errCode >= 0) {
        return nullptr;
    }
    return Mailbox::errMessages[~errCode];
}
