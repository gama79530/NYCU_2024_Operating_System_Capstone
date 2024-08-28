#ifndef FRAME_H
#define FRAME_H
#include "types.h"

typedef void* (*startup_malloc_callback_t)(uint64_t size);
typedef void (*startup_preserve_memory_callback_t)(void *metadata);

void* buddy_system_init(
    void *memory_base,
    void *memory_boundary,
    uint8_t frame_order,
    uint8_t buddy_order_limit,
    startup_malloc_callback_t malloc_cb,
    startup_preserve_memory_callback_t preserve_cb
);

bool buddy_preserve_memory(void *metadata, void *memory_base, void *memory_boundary, char *msg);
void buddy_show_layout(void *metadata);
void buddy_show_frame_state(void *metadata, uint64_t frame_idx);
void* buddy_frame_idx_to_addr(void *metadata, uint64_t frame_idx);
uint64_t buddy_addr_to_frame_idx(void *metadata, void *addr);

void* frame_alloc(void *metadata, uint8_t buddy_order);
void frame_free(void *metadata, void *ptr);

#endif