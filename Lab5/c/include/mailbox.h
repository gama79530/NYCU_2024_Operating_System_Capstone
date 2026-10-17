#ifndef LAB5_C_MAILBOX_H
#define LAB5_C_MAILBOX_H

#include "types.h"

typedef enum {
    MAILBOX_SUCCESS = 0,
    MAILBOX_ERROR_INVALID_ARGUMENT = -1,
    MAILBOX_ERROR_WRITE_TIMEOUT = -2,
    MAILBOX_ERROR_READ_TIMEOUT = -3,
    MAILBOX_ERROR_RESPONSE = -4,
    MAILBOX_ERROR_TAG_RESPONSE = -5,
} mailbox_error_t;

/** Send a property-channel (8) message and copy the response back into buffer.
 * Buffer must be 16-byte aligned and contain a complete, bounded tag sequence.
 * IRQ is masked during the bounded transaction to serialize the response FIFO.
 * Uses a private 16-byte-aligned copy and memory barriers with the MMU disabled.
 * Cache maintenance must be added when the buffer becomes cacheable.
 * A response timeout retains that copy until a later call drains the late reply. */
mailbox_error_t mailbox_call(unsigned char channel, volatile uint32_t *buffer);

/* Query the board revision through the mailbox property interface. */
mailbox_error_t mailbox_get_board_revision(uint32_t *revision);

/* Query the ARM memory base and size through the mailbox property interface. */
mailbox_error_t mailbox_get_arm_memory(uint32_t *base, uint32_t *size);

/* Convert a mailbox_error_t value into a user-facing error string. */
const char *mailbox_error_string(mailbox_error_t error);

#endif
