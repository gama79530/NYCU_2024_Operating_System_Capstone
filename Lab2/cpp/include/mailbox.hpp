#ifndef LAB2_CPP_MAILBOX_HPP
#define LAB2_CPP_MAILBOX_HPP
#include "types.hpp"

#define MAILBOX_BUFFER_SIZE 36
#define CHANNEL_MASK 0xF
#define TIMEOUT_MAX 1000000

class Mailbox
{
public:
    enum ReturnCode {
        MAILBOX_SUCCESS = 0,
        MAILBOX_ERR_RESPONSE = -1,
        MAILBOX_ERR_TIMEOUT = -2,
    };
    volatile uint32_t __attribute__((aligned(16))) buffer[MAILBOX_BUFFER_SIZE];

    Mailbox();
    ReturnCode mailboxExchange(uint8_t channel, volatile uint32_t *buffer);
    ReturnCode mailboxExchange(uint8_t channel);
    const char *errorCodeToStr(int32_t errCode);

private:
    static const char *const errMessages[];
};

#endif
