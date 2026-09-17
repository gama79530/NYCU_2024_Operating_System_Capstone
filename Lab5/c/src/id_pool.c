#include "id_pool.h"

#include "allocator.h"

/* Private constants */

#define ID_POOL_HEAD 0
#define ID_POOL_TAIL 1
#define ID_POOL_END ID_POOL_TAIL
#define ID_POOL_FIRST 2
#define ID_POOL_ALLOCATED 0

/* Function implementations */

id_pool_t *id_pool_create(size_t slot_count)
{
    id_pool_t *pool;

    if (slot_count < 3 || slot_count > 65536) {
        return NULL;
    }
    pool = malloc(sizeof(*pool) + slot_count * sizeof(pool->links[0]));
    if (pool == NULL) {
        return NULL;
    }

    pool->slot_count = slot_count;
    pool->links[ID_POOL_HEAD] = ID_POOL_FIRST;
    pool->links[ID_POOL_TAIL] = slot_count - 1;
    for (size_t slot = ID_POOL_FIRST; slot < slot_count - 1; slot++) {
        pool->links[slot] = slot + 1;
    }
    pool->links[slot_count - 1] = ID_POOL_END;
    return pool;
}

int id_pool_alloc(id_pool_t *pool)
{
    uint16_t slot = pool->links[ID_POOL_HEAD];

    if (slot == ID_POOL_END) {
        return -1;
    }
    pool->links[ID_POOL_HEAD] = pool->links[slot];
    /* Free links are END or a valid ID, so zero marks an allocated slot. */
    pool->links[slot] = ID_POOL_ALLOCATED;
    return slot;
}

bool id_pool_release(id_pool_t *pool, int id)
{
    size_t slot = (size_t) id;

    if (id < ID_POOL_FIRST || slot >= pool->slot_count) {
        return false;
    }
    if (pool->links[slot] != ID_POOL_ALLOCATED) {
        return false;
    }
    pool->links[slot] = ID_POOL_END;
    /* When empty, tail is unspecified; the returned slot becomes the head. */
    if (pool->links[ID_POOL_HEAD] == ID_POOL_END) {
        pool->links[ID_POOL_HEAD] = slot;
    } else {
        pool->links[pool->links[ID_POOL_TAIL]] = slot;
    }
    pool->links[ID_POOL_TAIL] = slot;
    return true;
}
