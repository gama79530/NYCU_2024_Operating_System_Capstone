#ifndef LAB4_C_BUDDY_H
#define LAB4_C_BUDDY_H

#include "config.h"
#include "types.h"

#define BUDDY_PAGE_SIZE (1UL << CONFIG_BUDDY_PAGE_SHIFT)
#define BUDDY_ORDER_COUNT (CONFIG_BUDDY_MAX_ORDER + 1)

typedef enum {
    BUDDY_SUCCESS = 0,
    BUDDY_ERROR_ALREADY_INITIALIZED = -1,
    BUDDY_ERROR_INVALID_RANGE = -2,
    BUDDY_ERROR_UNALIGNED_RANGE = -3,
    BUDDY_ERROR_METADATA_ALLOCATION_FAILED = -4,
    BUDDY_ERROR_NOT_RESERVING = -5,
    BUDDY_ERROR_NOT_READY = -6,
    BUDDY_ERROR_REQUEST_TOO_LARGE = -7,
    BUDDY_ERROR_OUT_OF_MEMORY = -8,
    BUDDY_ERROR_INVALID_ARGUMENT = -9,
    BUDDY_ERROR_ADDRESS_OUT_OF_RANGE = -10,
    BUDDY_ERROR_UNALIGNED_ADDRESS = -11,
    BUDDY_ERROR_NOT_ALLOCATED = -12,
} buddy_error_t;

typedef enum {
    BUDDY_STATE_UNINITIALIZED = 0,
    BUDDY_STATE_RESERVING,
    BUDDY_STATE_READY,
} buddy_state_t;

/* Return a stable description for a buddy allocator result. */
const char *buddy_error_string(buddy_error_t error);

/* Allocate frame metadata and enter the reservation phase. */
buddy_error_t buddy_init(uintptr_t memory_base, uintptr_t memory_end);

/* Mark every page touched by [begin, end) as unavailable. */
buddy_error_t buddy_reserve(uintptr_t begin, uintptr_t end);

/* Build maximal free blocks and make the allocator ready. */
buddy_error_t buddy_build(void);

/* Allocate page_count physically contiguous pages and return unused suffix pages. */
buddy_error_t buddy_alloc_pages(size_t page_count, void **address);

/* Return an exact allocation; page_count must match the original request. */
buddy_error_t buddy_free_pages(void *address, size_t page_count);

/* Return the allocator lifecycle state. */
buddy_state_t buddy_get_state(void);

/* Print managed range and the number of free blocks in each order. */
void buddy_dump_state(void);

#endif
