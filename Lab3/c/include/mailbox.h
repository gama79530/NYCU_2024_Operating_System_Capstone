#ifndef LAB3_C_MAILBOX_H
#define LAB3_C_MAILBOX_H
#include "types.h"

#define MAILBOX_BUFFER_SIZE 36
#define CHANNEL_MASK 0xF
#define TIMEOUT_MAX 1000000

#define MAILBOX_SUCCESS 0
#define MAILBOX_ERR_RESPONSE -1
#define MAILBOX_ERR_TIMEOUT -2

volatile uint32_t *get_default_buffer(void);
int32_t mailbox_exchange(uint8_t channel, volatile uint32_t *buffer);
const char* err_code_to_str(int32_t err_code);

#endif
