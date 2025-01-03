#include "frame.h"
#include "list.h"
#include "config.h"
#include "memory.h"
#include "printf.h"
#include "string.h"
#include "util.h"

#define BUDDY_STATE_PRESERVED       ((int8_t)0x80)
#define BUDDY_STATE_ALLOCATED       ((int8_t)0x81)
#define BUDDY_STATE_BUDDY_GROUPED   ((int8_t)0x82)
#define BUDDY_STATE_ERROR           ((int8_t)0x83)

static int8_t *buddy_order_array = NULL;
static list_head_t *buddy_group_lists = NULL;

/************************************************************************************
 * 0: before initialize
 * 1: after initialize and before build
 * 2: after build
 ************************************************************************************/
static uint8_t buddy_sys_state = 0;

static bool group_buddy(uint64_t *frame_idx_ptr);
static int8_t get_buddy_order(uint64_t frame_idx);

int buddy_sys_init(void){
    if(buddy_sys_state > 0){
#if VERBOSE != 0
        printf("buddy_sys_init: can not initialize buddy system twice.\n");
#endif
        return -1;
    }

    buddy_order_array = (int8_t*)startup_alloc(FRAME_NUM * sizeof(int8_t));
    if(buddy_order_array == NULL){
#if VERBOSE != 0
        printf("buddy_sys_init: buddy_order_array allocation fail.\n");
#endif
        return -1;
    }
    for(uint64_t i = 0; i < FRAME_NUM; i++){
        buddy_order_array[i] = 0;
    }

    buddy_group_lists = (list_head_t*)startup_alloc(BUDDY_GROUP_ORDER_LIMIT * sizeof(list_head_t));
    if(buddy_group_lists == NULL){
#if VERBOSE != 0
        printf("buddy_sys_init: buddy_group_lists allocation fail.\n");
#endif
        return -1;
    }
    for(uint64_t i = 0; i < BUDDY_GROUP_ORDER_LIMIT; i++){
        buddy_group_lists[i].prev = buddy_group_lists[i].next = buddy_group_lists + i;
    }

    buddy_sys_state = 1;

    return 0;
}

int buddy_sys_preserve_memory(void *base, void *boundary, char *msg){
    /* buddy system state check */
    if(buddy_sys_state < 1){
#if VERBOSE != 0
        printf("buddy_sys_preserve_memory: preserve memory before initialization is not allowed.\n");
#endif
        return -1;
    }else if(buddy_sys_state > 1){
#if VERBOSE != 0
        printf("buddy_sys_preserve_memory: preserve memory after build is not allowed.\n");
#endif
        return -1;
    }

    /* preserve range check */
    if((uint64_t)base < MEMORY_BASE || (uint64_t)boundary > MEMORY_BOUNDARY){
#if VERBOSE == true
        printf("buddy_sys_preserve_memory: out of memory bounds\n");
#endif
        return -1;
    }

    uint64_t begin_frame_idx = address_to_frame_idx(base);
    uint64_t end_frame_idx = address_to_frame_idx(boundary - 1);
    for(uint64_t i = begin_frame_idx; i <= end_frame_idx; i++){
        buddy_order_array[i] = BUDDY_STATE_PRESERVED;
    }

#if VERBOSE != 0
    if(msg != NULL){
        printf("%s (range: 0x%s", msg, uint_to_hex_str((uint64_t)base, 8, NULL));
        printf(
            " - 0x%s, begin_frame_idx = %d, end_frame_idx = %d).\n", 
            uint_to_hex_str((uint64_t)boundary, 8, NULL), 
            begin_frame_idx, 
            end_frame_idx
        );
    }
#endif

    return 0;
}

int buddy_sys_build(void){
    if(buddy_sys_state < 1){
#if VERBOSE != 0
        printf("buddy_sys_build: can not build buddy system before initialization.\n");
#endif
        return -1;
    }else if(buddy_sys_state > 1){
#if VERBOSE != 0
        printf("buddy_sys_build: can not build buddy system twice.\n");
#endif
        return -1;
    }

    for(int8_t buddy_order = 0; buddy_order < BUDDY_GROUP_ORDER_LIMIT; buddy_order++){
        for(uint64_t frame_idx = 0; frame_idx < FRAME_NUM; frame_idx += (2 << buddy_order)){
            group_buddy(&frame_idx);
        }
    }

    for(uint64_t frame_idx = 0; frame_idx < FRAME_NUM; frame_idx++){
        if(buddy_order_array[frame_idx] < 0) continue;

        list_head_t *node = (list_head_t*)frame_idx_to_address(frame_idx);
        list_add_last(node, buddy_group_lists + buddy_order_array[frame_idx]);
    }

    buddy_sys_state = 2;

    return 0;
}

void* frame_idx_to_address(uint64_t frame_idx){
    return frame_idx < FRAME_NUM ? (void*)(MEMORY_BASE + (frame_idx << FRAME_ORDER)) : NULL;    
}

uint64_t address_to_frame_idx(void *ptr){
    uint64_t address = (uint64_t)ptr;
    return address >= MEMORY_BASE && address < MEMORY_BOUNDARY ? (address - MEMORY_BASE) >> FRAME_ORDER : 0;
}

static bool group_buddy(uint64_t *frame_idx_ptr){
    if(buddy_sys_state == 0)    return false;
    uint64_t frame_idx, buddy_idx;
    
    frame_idx = *frame_idx_ptr;

    if(frame_idx >= FRAME_NUM || buddy_order_array[frame_idx] < 0 || 
      buddy_order_array[frame_idx] >= BUDDY_GROUP_ORDER_LIMIT - 1){
        return false;
    }

    buddy_idx = frame_idx ^ (1UL << (uint64_t)buddy_order_array[frame_idx]);
    if(frame_idx > buddy_idx)   swap(frame_idx, buddy_idx);

    if(buddy_idx >= FRAME_NUM || buddy_order_array[frame_idx] != buddy_order_array[buddy_idx]){
        return false;
    }

    buddy_order_array[frame_idx]++;
    buddy_order_array[buddy_idx] = BUDDY_STATE_BUDDY_GROUPED;

    if(buddy_sys_state > 1){
        list_head_t *frame_node = (list_head_t*)frame_idx_to_address(frame_idx);
        list_head_t *buddy_node = (list_head_t*)frame_idx_to_address(buddy_idx);

        list_remove(frame_node);
        list_remove(buddy_node);
        list_add_last(frame_node, buddy_group_lists + buddy_order_array[frame_idx]);

#if VERBOSE != 0
        printf("group_buddy: group buddy %s", uint_to_dec_str(frame_idx, NULL));
        printf(" and buddy %s", uint_to_dec_str(buddy_idx, NULL));
        printf(
            " to make new contiguous buddy group with order %s.\n", 
            uint_to_dec_str(buddy_order_array[frame_idx], NULL)
        );
#endif
    }

    *frame_idx_ptr = frame_idx;

    return true;
}

void buddy_sys_show_layout(void){
    if(buddy_sys_state < 2){
        printf("buddy_sys_show_layout: the buddy system is not built yet.\n");
        return;   
    }

    printf("\n********** buddy_sys_show_layout **********\n");
    printf("memory_base\t: 0x%s\n", uint_to_hex_str(MEMORY_BASE, 8, NULL));
    printf("memory_boundary\t: 0x%s\n", uint_to_hex_str(MEMORY_BOUNDARY, 8, NULL));
    printf("frame_size\t: 2^%d bytes\n", FRAME_ORDER);
    printf("frame_num\t: %d\n\n", FRAME_NUM);

    uint64_t frame_idx = 0;
    while(frame_idx < FRAME_NUM){
        buddy_sys_show_frame_state(frame_idx);

        if(buddy_order_array[frame_idx] == BUDDY_STATE_ALLOCATED || buddy_order_array[frame_idx] >= 0){
            frame_idx += (1 << get_buddy_order(frame_idx));
        }else if(buddy_order_array[frame_idx] == BUDDY_STATE_PRESERVED){
            while(++frame_idx < FRAME_NUM && buddy_order_array[frame_idx] == BUDDY_STATE_PRESERVED);
            buddy_sys_show_frame_state(frame_idx - 1);
        }else{
            frame_idx++;
        }
    }
    
    printf("\n********** buddy_sys_show_layout **********\n");
}

void buddy_sys_show_frame_state(uint64_t frame_idx){
    if(buddy_sys_state < 2){
        printf("buddy_sys_show_frame_state: the buddy system is not built yet.\n");
        return;   
    }

    printf("The state of frame %d is ", frame_idx);
    if(buddy_order_array[frame_idx] == BUDDY_STATE_ALLOCATED){
        printf("STATE_ALLOCATED\n");
    }else if(buddy_order_array[frame_idx] == BUDDY_STATE_BUDDY_GROUPED){
        printf("BUDDY_STATE_BUDDY_GROUPED\n");
    }else if(buddy_order_array[frame_idx] == BUDDY_STATE_PRESERVED){
        printf("BUDDY_STATE_PRESERVED\n");
    }else{
        printf("at order %d\n", buddy_order_array[frame_idx]);
    }
}

void* frame_alloc(uint8_t buddy_order){
    void *ret_frame_ptr = NULL;
    uint64_t ret_frame_idx;
    if(buddy_sys_state < 2){
#if VERBOSE != 0
        printf("frame_alloc: the buddy system is not built yet.\n");
#endif
    // over the buddy system's limit
    }else if(buddy_order >= BUDDY_GROUP_ORDER_LIMIT){
#if VERBOSE != 0
        printf(
            "frame_alloc: request buddy_order (= %d) is greater than or equal to the limit (= %d).\n",
            buddy_order,
            BUDDY_GROUP_ORDER_LIMIT
        );
#endif
    // find contiguous frame directly
    }else if(!list_is_empty(buddy_group_lists + buddy_order)){
        list_head_t *node = buddy_group_lists[buddy_order].next;
        list_remove(node);
        ret_frame_ptr = (void*)node;
        ret_frame_idx = address_to_frame_idx(ret_frame_ptr);
        buddy_order_array[ret_frame_idx] = BUDDY_STATE_ALLOCATED;

#if VERBOSE != 0
        printf(
            "frame_alloc: find contiguous buddy group %d with order %d directly.\n",
            ret_frame_idx,
            buddy_order
        );
#endif
    // try to split buddy group with greater order
    }else{
        ret_frame_ptr = frame_alloc(buddy_order + 1);
        if(ret_frame_ptr != NULL){
            ret_frame_idx = address_to_frame_idx(ret_frame_ptr);
            uint64_t buddy_frame_idx = ret_frame_idx ^ (1 << buddy_order);
            void *buddy_frame_ptr = frame_idx_to_address(buddy_frame_idx);

            buddy_order_array[buddy_frame_idx] = buddy_order;
            list_add_last((list_head_t*)buddy_frame_ptr, buddy_group_lists + buddy_order);

#if VERBOSE != 0
            printf(
                "frame_alloc: find contiguous buddy group by split buddy group with order %d.\n", 
                buddy_order + 1
            );
#endif
        }
    }

    return ret_frame_ptr;
}

void frame_free(void *ptr){
    if(buddy_sys_state < 2){
#if VERBOSE != 0
        printf("frame_free: the buddy system is not built yet.\n");
#endif
        return;
    /* out of range */
    }else if((uint64_t)ptr < MEMORY_BASE || (uint64_t)ptr >= MEMORY_BOUNDARY){
#if VERBOSE != 0
        printf("frame_free: out of memory bounds.\n");
#endif
        return;
    } 

    uint64_t frame_idx = address_to_frame_idx(ptr);
    ptr = frame_idx_to_address(frame_idx);
    if(buddy_order_array[frame_idx] != BUDDY_STATE_ALLOCATED){
#if VERBOSE != 0
        printf("frame_free: buddy group %d is not allocated.\n", frame_idx);
#endif
        return;
    }

#if VERBOSE != 0
    printf("frame_free: return buddy group %d.\n", frame_idx);
#endif

    int8_t buddy_order = get_buddy_order(frame_idx);

    buddy_order_array[frame_idx] = buddy_order;
    list_add_last((list_head_t*)ptr, buddy_group_lists + buddy_order);
    while(group_buddy(&frame_idx));
}

static int8_t get_buddy_order(uint64_t frame_idx){
    int8_t buddy_order = 0; 
    /* need check */
    if(frame_idx >= FRAME_NUM || buddy_sys_state < 1){
        buddy_order = BUDDY_STATE_ERROR;
    }else if(buddy_order_array[frame_idx] == BUDDY_STATE_PRESERVED || buddy_order_array[frame_idx] >= 0){
        buddy_order = buddy_order_array[frame_idx];
    }else if(buddy_order_array[frame_idx] == BUDDY_STATE_ALLOCATED || 
             buddy_order_array[frame_idx] == BUDDY_STATE_BUDDY_GROUPED){
        uint64_t buddy_idx = frame_idx ^ (1 << buddy_order);
        while(buddy_order_array[buddy_idx] == BUDDY_STATE_BUDDY_GROUPED){
            if(frame_idx > buddy_idx)   
                swap(frame_idx, buddy_idx);
            buddy_idx = frame_idx ^ (1 << ++buddy_order);
        }
    }else{
        buddy_order = BUDDY_STATE_ERROR;
    }

    return buddy_order;
}

uint8_t get_buddy_sys_state(void){
    return buddy_sys_state;
}