#ifndef __MAILBOX_HPP__
#define __MAILBOX_HPP__
#include "types.hpp"

#define MAILBOX_BUFFER_SIZE 36
#define CHANNEL_MASK 0xF
#define TIMEOUT_MAX 1000000

// #define MAILBOX_SUCCESS 0
// #define MAILBOX_ERR_RESPONSE -1
// #define MAILBOX_ERR_TIMEOUT -2

// volatile uint32_t *get_default_buffer(void);
// int32_t mailbox_exchange(uint8_t channel, volatile uint32_t *buffer);
// const char* err_code_to_str(int32_t err_code);

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
