#ifndef __FRAME_H__
#define __FRAME_H__
#include "types.h"

int buddy_sys_init(void);
int buddy_sys_preserve_memory(void *base, void *boundary, char *msg);
int buddy_sys_build(void);

/*****************************************************************
 * buddy_sys_state value:
 *  0: before initialize
 *  1: after initialize and before build
 *  2: after build
 *****************************************************************/
uint8_t get_buddy_sys_state(void);

void* frame_idx_to_address(uint64_t frame_idx);
uint64_t address_to_frame_idx(void *ptr);
uint64_t find_buddy_idx(uint64_t frame_idx, int8_t order);
uint64_t split_buddy_group(uint64_t frame_idx, int8_t order);

void buddy_sys_show_layout(void);
void buddy_sys_show_frame_state(uint64_t frame_idx);

void* frame_alloc(uint8_t buddy_order);
void frame_free(void *ptr);

#endif