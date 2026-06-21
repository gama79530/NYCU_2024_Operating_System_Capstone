#include "mailbox.h"

#include "config.h"
#include "error.h"
#include "log.h"
#include "peripheral.h"
#include "util.h"

/* Private types */

/* Private function declarations */
static mailbox_error_t mailbox_property_call(volatile uint32_t *buffer);
static mailbox_error_t mailbox_validate_tag(volatile uint32_t *buffer, uint32_t value_size);

/* Private data */
/*
 * Mailbox property interface
 * ref: https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 *
 * Buffer contents:
 *     u32: size in bytes, including the end tag
 *     u32: request code / response code
 *     u8 ...: sequence of concatenated tags
 *     u32: end tag
 *     u8 ...: padding
 *
 * Tag format:
 *     u32: tag identifier
 *     u32: value buffer size in bytes
 *     u32: request code / response code
 *     b31 = 0: request, b30-b0: reserved
 *     b31 = 1: response, b30-b0: value length in bytes
 *     u8 ...: value buffer
 *     u8 ...: padding to align to 32 bits
 */
#define MAILBOX_REQUEST 0x00000000
#define MAILBOX_RESPONSE_SUCCESS 0x80000000
#define MAILBOX_RESPONSE_ERROR 0x80000001

/* Channels */
#define MAILBOX_CH_POWER 0
#define MAILBOX_CH_FB 1
#define MAILBOX_CH_VUART 2
#define MAILBOX_CH_VCHIQ 3
#define MAILBOX_CH_LEDS 4
#define MAILBOX_CH_BTNS 5
#define MAILBOX_CH_TOUCH 6
#define MAILBOX_CH_COUNT 7
#define MAILBOX_CH_PROP 8

/* Tags */
#define MAILBOX_TAG_GETFWREVISION 0x00000001  // u32: firmware revision
#define MAILBOX_TAG_GETMODEL 0x00010001       // u32: board model
#define MAILBOX_TAG_GETREVISION 0x00010002    // u32: board revision
#define MAILBOX_TAG_GETMACADDR 0x00010003     // u8 * 6: MAC address in network byte order
#define MAILBOX_TAG_GETSERIAL 0x00010004      // u64: board serial
#define MAILBOX_TAG_GETMEMORY 0x00010005      // u32: base, u32: size
#define MAILBOX_TAG_GETVCMEM 0x00010006       // u32: base, u32: size
#define MAILBOX_TAG_GETCLOCKS 0x00010007      // u32: parent clock id, u32: clock id, repeated

#define MAILBOX_TAG_ALLOCATEFB 0x00040001  // u32: alignment in bytes -> u32: base, u32: size

#define MAILBOX_TAG_GETPHYSICALWH 0x00040003     // u32: width, u32: height
#define MAILBOX_TAG_GETVIRTUALWH 0x00040004      // u32: width, u32: height
#define MAILBOX_TAG_GETDEPTH 0x00040005          // u32: bits per pixel
#define MAILBOX_TAG_GETPIXELORDER 0x00040006     // u32: 0x0=BGR, 0x1=RGB
#define MAILBOX_TAG_GETALPHAMODE 0x00040007      // u32: 0x0=enabled, 0x1=reversed, 0x2=ignored
#define MAILBOX_TAG_GETPITCH 0x00040008          // u32: bytes per line
#define MAILBOX_TAG_GETVIRTUALOFFSET 0x00040009  // u32: X, u32: Y
#define MAILBOX_TAG_GETOVERSCAN 0x0004000A
#define MAILBOX_TAG_GETPALETTE 0x0004000B

#define MAILBOX_TAG_REQUEST 0x00000000
#define MAILBOX_TAG_RESPONSE 0x80000000
#define MAILBOX_TAG_RESPONSE_LENGTH_MASK 0x7FFFFFFF
#define MAILBOX_TAG_END 0x00000000

#define MAILBOX_BUFFER_WORDS 36
#define MAILBOX_ERROR_COUNT (sizeof(mailbox_error_messages) / sizeof(mailbox_error_messages[0]))

/* The current single-core kernel serializes access to this shared buffer. */
static volatile uint32_t mailbox_buffer[MAILBOX_BUFFER_WORDS] __attribute__((aligned(16)));

static const char *const mailbox_error_messages[] = {
    "Invalid argument or mailbox buffer.",
    "Timed out waiting to write mailbox request.",
    "Timed out waiting for mailbox response.",
    "Firmware rejected the mailbox request.",
    "Firmware returned an invalid tag response.",
};

/* Function implementations */
mailbox_error_t mailbox_get_board_revision(uint32_t *revision)
{
    mailbox_error_t error;

    if (revision == NULL) {
        return MAILBOX_ERROR_INVALID_ARGUMENT;
    }

    mailbox_buffer[0] = 7 * sizeof(uint32_t);
    mailbox_buffer[1] = MAILBOX_REQUEST;
    mailbox_buffer[2] = MAILBOX_TAG_GETREVISION;
    mailbox_buffer[3] = sizeof(uint32_t);
    mailbox_buffer[4] = MAILBOX_TAG_REQUEST;
    mailbox_buffer[5] = 0;
    mailbox_buffer[6] = MAILBOX_TAG_END;

    LOG_VERBOSE("mailbox", "query board revision");
    error = mailbox_property_call(mailbox_buffer);
    if (error != MAILBOX_SUCCESS) {
        return error;
    }

    error = mailbox_validate_tag(mailbox_buffer, sizeof(uint32_t));
    if (error != MAILBOX_SUCCESS) {
        return error;
    }

    *revision = mailbox_buffer[5];
    LOG_VERBOSE("mailbox", "board revision = 0x%08X", *revision);
    return MAILBOX_SUCCESS;
}

mailbox_error_t mailbox_get_arm_memory(uint32_t *base, uint32_t *size)
{
    mailbox_error_t error;

    if (base == NULL || size == NULL) {
        return MAILBOX_ERROR_INVALID_ARGUMENT;
    }

    mailbox_buffer[0] = 8 * sizeof(uint32_t);
    mailbox_buffer[1] = MAILBOX_REQUEST;
    mailbox_buffer[2] = MAILBOX_TAG_GETMEMORY;
    mailbox_buffer[3] = 2 * sizeof(uint32_t);
    mailbox_buffer[4] = MAILBOX_TAG_REQUEST;
    mailbox_buffer[5] = 0;
    mailbox_buffer[6] = 0;
    mailbox_buffer[7] = MAILBOX_TAG_END;

    LOG_VERBOSE("mailbox", "query ARM memory");
    error = mailbox_property_call(mailbox_buffer);
    if (error != MAILBOX_SUCCESS) {
        return error;
    }

    error = mailbox_validate_tag(mailbox_buffer, 2 * sizeof(uint32_t));
    if (error != MAILBOX_SUCCESS) {
        return error;
    }

    *base = mailbox_buffer[5];
    *size = mailbox_buffer[6];
    LOG_VERBOSE("mailbox", "ARM memory base = 0x%08X, size = 0x%08X", *base, *size);
    return MAILBOX_SUCCESS;
}

const char *mailbox_error_string(mailbox_error_t error)
{
    size_t index;

    if (error >= MAILBOX_SUCCESS) {
        return "Unknown mailbox error.";
    }

    index = error_code_to_index(error);
    if (index >= MAILBOX_ERROR_COUNT) {
        return "Unknown mailbox error.";
    }

    return mailbox_error_messages[index];
}

static mailbox_error_t mailbox_property_call(volatile uint32_t *buffer)
{
    uintptr_t address = (uintptr_t) buffer;
    uint32_t request;
    size_t attempts;

    if (buffer == NULL || (address & 0x0f) != 0 || address > UINT32_MAX) {
        return MAILBOX_ERROR_INVALID_ARGUMENT;
    }

    request = ((uint32_t) address & ~0x0fu) | MAILBOX_CH_PROP;
    LOG_VERBOSE("mailbox", "request = 0x%08X", request);

    for (attempts = 0; attempts < CONFIG_MAILBOX_TIMEOUT; attempts++) {
        if ((get32(MAILBOX_STATUS) & MAILBOX_STATUS_FULL) == 0) {
            break;
        }
    }

    if (attempts == CONFIG_MAILBOX_TIMEOUT) {
        LOG_VERBOSE("mailbox", "write timeout");
        return MAILBOX_ERROR_WRITE_TIMEOUT;
    }

    data_memory_barrier();
    put32(MAILBOX_WRITE, request);

    for (attempts = 0; attempts < CONFIG_MAILBOX_TIMEOUT; attempts++) {
        uint32_t response;

        if ((get32(MAILBOX_STATUS) & MAILBOX_STATUS_EMPTY) != 0) {
            continue;
        }

        response = get32(MAILBOX_READ);
        if (response != request) {
            continue;
        }

        data_memory_barrier();
        if (buffer[1] != MAILBOX_RESPONSE_SUCCESS) {
            LOG_VERBOSE("mailbox", "response code = 0x%08X", buffer[1]);
            return MAILBOX_ERROR_RESPONSE;
        }

        LOG_VERBOSE("mailbox", "response accepted");
        return MAILBOX_SUCCESS;
    }

    LOG_VERBOSE("mailbox", "read timeout");
    return MAILBOX_ERROR_READ_TIMEOUT;
}

static mailbox_error_t mailbox_validate_tag(volatile uint32_t *buffer, uint32_t value_size)
{
    uint32_t response = buffer[4];
    uint32_t response_size = response & MAILBOX_TAG_RESPONSE_LENGTH_MASK;

    if ((response & MAILBOX_TAG_RESPONSE) == 0 ||
        response_size < value_size || response_size > buffer[3]) {
        LOG_VERBOSE("mailbox", "invalid tag response = 0x%08X", response);
        return MAILBOX_ERROR_TAG_RESPONSE;
    }

    return MAILBOX_SUCCESS;
}
