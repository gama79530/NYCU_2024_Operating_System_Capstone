#ifndef LAB5_C_ID_POOL_H
#define LAB5_C_ID_POOL_H

#include "types.h"

/* Slots 0 and 1 store head/tail; remaining slots represent consecutive IDs.
 * Use a fixed link width so independent pools can have different capacities. */
typedef struct {
    size_t slot_count; /* Includes the two head/tail metadata slots. */
    uint16_t links[];
} id_pool_t;

/**
 * Allocate and initialize 3..65536 slots; return NULL on invalid size or failure.
 * Slots 0 and 1 hold metadata; IDs equal indices 2 through slot_count - 1.
 * Callers needing zero-based IDs must subtract 2 after allocation succeeds and
 * add 2 before release. The usable capacity is slot_count - 2.
 * Free the returned pool only after the pool and its IDs are no longer used.
 * Operations on a shared pool require caller-provided synchronization
 * (e.g. IRQ masking); alloc/release require a non-NULL pool from create.
 */
id_pool_t *id_pool_create(size_t slot_count);

/** Pop the oldest free ID in O(1); return -1 when the initialized pool is empty. */
int id_pool_alloc(id_pool_t *pool);

/**
 * Append an allocated ID in O(1); return false if it is out of range or
 * already free. Valid IDs are 2..slot_count - 1.
 * A stale ID reused by a new owner cannot be distinguished from that owner's ID.
 */
bool id_pool_release(id_pool_t *pool, int id);

#endif
