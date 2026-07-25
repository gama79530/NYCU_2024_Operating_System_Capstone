#ifndef LAB3_C_MAILBOX_H
#define LAB3_C_MAILBOX_H

#include "types.h"

typedef enum {
    MAILBOX_SUCCESS = 0,
    MAILBOX_ERROR_INVALID_ARGUMENT = -1,
    MAILBOX_ERROR_WRITE_TIMEOUT = -2,
    MAILBOX_ERROR_READ_TIMEOUT = -3,
    MAILBOX_ERROR_RESPONSE = -4,
    MAILBOX_ERROR_TAG_RESPONSE = -5,
} mailbox_error_t;

/* Query the board revision through the mailbox property interface. */
mailbox_error_t mailbox_get_board_revision(uint32_t *revision);

/* Query the ARM memory base and size through the mailbox property interface. */
mailbox_error_t mailbox_get_arm_memory(uint32_t *base, uint32_t *size);

/* Convert a mailbox_error_t value into a user-facing error string. */
const char *mailbox_error_string(mailbox_error_t error);

#endif
