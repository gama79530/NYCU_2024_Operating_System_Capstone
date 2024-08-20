#include "frame.h"
#include "util.h"
#include "memory.h"
#include "list.h"
#include "mini_uart.h"
#include "string.h"
#include "dtb.h"
#include "cpio.h"
#include "util.h"

extern char kernel_begin;
extern char kernel_end;
extern void *startup_heap_base;
extern void *startup_heap_boundary;

/* buddy system */
#define BUDDY_ORDER_LIMIT           16       // exclusive
#define STATE_PRESERVED             0x80
#define STATE_ALLOCATED             0x81
#define STATE_BUDDY                 0x82
#define BUDDY_GROUP_SUCCESS         0
#define BUDDY_GROUP_FAIL            1
#define BUDDY_UNGROUP_SUCCESS       0
#define BUDDY_UNGROUP_FAIL          1

/*  buddy system metadata 
    buddy_array[i] >= 0 : The order of the frame at index i.
    buddy_array[i] < 0  : The state of frame i.
*/

typedef struct buddy_sys_metadata{
    uint8_t     is_build;
    void        *base_ptr;
    void        *boundary_ptr;
    uint64_t    frame_num;
    int8_t      *buddy_array;
    list_head_t *buddy_lists;
} buddy_sys_metadata_t;

static buddy_sys_metadata_t *metadata;

void buddy_preserve_memory(void *base_ptr, void *boundary_ptr, char *msg);
int buddy_group(uint64_t *frame_idx_ptr, bool print_msg);
int buddy_ungroup(uint64_t frame_idx, bool print_msg);

void show_memory_info();
uint64_t addr_to_idx(void *addr);
void *idx_to_addr(uint64_t idx);

int buddy_system_init(){
#if PRINT_MSG == 1
    uart_poll_putln("");
    uart_poll_putln("********** buddy_system_init **********");
#endif

    /* initialize buddy system metadata */
    metadata = (buddy_sys_metadata_t*)startup_memory_alloc(sizeof(buddy_sys_metadata_t));
    if(metadata == NULL){
        uart_poll_putln("buddy_system_init: metadata allocation fail.");
        return -1;    
    }

    metadata->is_build = 0;

    /* hard coding but they can be replaced by the value in dtb */
    metadata->base_ptr = (void*)MEMORY_BASE;
    metadata->boundary_ptr = (void*)MEMORY_BOUNDARY;
    metadata->frame_num = ((uint64_t)metadata->boundary_ptr - (uint64_t)metadata->base_ptr) >> FRAME_SIZE_ORDER;

    metadata->buddy_array = (int8_t*)startup_memory_alloc(metadata->frame_num * sizeof(int8_t));
    if(metadata->buddy_array == NULL){
        uart_poll_putln("buddy_system_init: buddy_array allocation fail.");
        return -1;    
    }
    for(uint64_t i = 0; i < metadata->frame_num; i++){
        metadata->buddy_array[i] = 0;
    }
    
    metadata->buddy_lists = (list_head_t*)startup_memory_alloc(BUDDY_ORDER_LIMIT * sizeof(list_head_t));
    if(metadata->buddy_lists == NULL){
        uart_poll_putln("buddy_system_init: buddy_lists allocation fail.");
        return -1;    
    }
    for(int i = 0; i < BUDDY_ORDER_LIMIT; i++){
        metadata->buddy_lists[i].prev = metadata->buddy_lists[i].next = metadata->buddy_lists + i;
    }

    /* preserve memory */
    buddy_preserve_memory((void*)0x0000, (void*)0x1000, "Preserve spin tables for multicore boot");
    buddy_preserve_memory((void*)&kernel_begin, (void*)&kernel_end, "Preserve kernel image space");
    buddy_preserve_memory(get_cpio_begin_ptr(), get_cpio_end_ptr(), "Preserve CPIO memory");
    buddy_preserve_memory(get_dtb_ptr(), get_dtb_ptr() + get_dtb_size(), "Preserve Device tree memory");
    buddy_preserve_memory(startup_heap_base, startup_heap_boundary, "Preserve startup heap");

    /* build buddy system */
    for(uint8_t order = 0; order < BUDDY_ORDER_LIMIT; order++){
        for(uint64_t frame_idx = 0; frame_idx < metadata->frame_num; frame_idx += (1 << order)){
            buddy_group(&frame_idx, true);
        }
    }



#if PRINT_MSG == 1
    show_memory_info();
#endif

    return 0;
}

void buddy_preserve_memory(void *base_ptr, void *boundary_ptr, char *msg){
    uint64_t begin_idx, end_idx;

    /* preserve range check */
    if((uint64_t)base_ptr < (uint64_t)metadata->base_ptr || 
       (uint64_t)boundary_ptr > (uint64_t)metadata->boundary_ptr){
        uart_poll_puts("Out of memory space");
        return;
    }

    begin_idx = addr_to_idx((void*)align_floor((uint64_t)base_ptr, FRAME_SIZE));
    end_idx = addr_to_idx((void*)align_floor((uint64_t)(boundary_ptr-1), FRAME_SIZE));

    for(uint64_t i = begin_idx; i <= end_idx; i++){
        metadata->buddy_array[i] = STATE_PRESERVED;
    }

#if PRINT_MSG == 1
    if(msg != NULL){
        uart_poll_puts(msg);
    }
    uart_poll_puts(" (range: 0x");
    uart_poll_puts(long_to_hex_str((uint64_t)base_ptr) + 8);
    uart_poll_puts(" - 0x");
    uart_poll_puts(long_to_hex_str((uint64_t)boundary_ptr) + 8);
    uart_poll_puts(", begin_idx = ");
    uart_poll_puts(uint_to_dec_str(begin_idx));
    uart_poll_puts(", end_idx = ");
    uart_poll_puts(uint_to_dec_str(end_idx));
    uart_poll_putln(")");
#endif
}

void* frame_alloc(uint64_t size){
    return NULL;
}

void frame_free(void *ptr){

}

void show_memory_info(){
    uart_poll_putln("");
    uart_poll_putln("********** show_memory_info **********");
    uart_poll_puts("base_ptr\t: 0x");
    uart_poll_putln(long_to_hex_str((uint64_t)metadata->base_ptr) + 8);

    uart_poll_puts("boundary_ptr\t: 0x");
    uart_poll_putln(long_to_hex_str((uint64_t)metadata->boundary_ptr) + 8);

    uart_poll_puts("frame_size\t: 2^" );
    uart_poll_puts(uint_to_dec_str(FRAME_SIZE_ORDER));
    uart_poll_putln(" bytes");

    uart_poll_puts("frame_num\t: " );
    uart_poll_putln(uint_to_dec_str(metadata->frame_num));
    
    uart_poll_putln("");
}

uint64_t addr_to_idx(void *addr){
    return ((uint64_t)addr - (uint64_t)metadata->base_ptr) >> FRAME_SIZE_ORDER;
}

void *idx_to_addr(uint64_t idx){
    return metadata->base_ptr + (idx << FRAME_SIZE_ORDER);
}

int buddy_group(uint64_t *frame_idx_ptr, bool print_msg){
    uint64_t frame_idx, buddy_idx;
    int32_t order;
    
    frame_idx = *frame_idx_ptr;
    order = metadata->buddy_array[frame_idx];
    
    if(order < 0 || order == BUDDY_ORDER_LIMIT - 1){
        return BUDDY_GROUP_FAIL;
    }

    buddy_idx = frame_idx ^ (1 << order);
    if(frame_idx > buddy_idx){
        swap(frame_idx, buddy_idx);
    }

    if(metadata->buddy_array[frame_idx] != metadata->buddy_array[buddy_idx]){
        return BUDDY_GROUP_FAIL;
    }
    
    metadata->buddy_array[frame_idx] = ++order;
    metadata->buddy_array[buddy_idx] = STATE_BUDDY;

    /* processing buddy_lists */
    if(metadata->is_build){
        list_head_t *frame_node = (list_head_t*)idx_to_addr(frame_idx);
        list_head_t *buddy_node = (list_head_t*)idx_to_addr(buddy_idx);

        list_remove(frame_node);
        list_remove(buddy_node);
        list_add(frame_node, metadata->buddy_lists[order].prev, &metadata->buddy_lists[order]);
    }

    *frame_idx_ptr = frame_idx;

    if(print_msg){
        uart_poll_puts("group frame ");
        uart_poll_puts(uint_to_dec_str(frame_idx));
        uart_poll_puts(" and frame ");
        uart_poll_puts(uint_to_dec_str(buddy_idx));
        uart_poll_puts(" to make new contiguous buddy with order ");
        uart_poll_putln(uint_to_dec_str(order));
    }
    
    return BUDDY_GROUP_SUCCESS;
}

int buddy_ungroup(uint64_t frame_idx, bool print_msg){
    int32_t order = metadata->buddy_array[frame_idx] - 1;
       
    if(order < 0){
        return BUDDY_UNGROUP_FAIL;
    }

    uint64_t buddy_idx = frame_idx ^ (1 << order);
    metadata->buddy_array[frame_idx] = metadata->buddy_array[buddy_idx] = order;

    /* processing buddy_lists */
    if(metadata->is_build){
        list_head_t *frame_node = (list_head_t*)idx_to_addr(frame_idx);
        list_head_t *buddy_node = (list_head_t*)idx_to_addr(buddy_idx);

        list_remove(frame_node);
        list_add(frame_node, metadata->buddy_lists[order].prev, &metadata->buddy_lists[order]);
        list_add(buddy_node, metadata->buddy_lists[order].prev, &metadata->buddy_lists[order]);
    }

    if(print_msg){
        uart_poll_puts("ungroup to make two new contiguous buddy with order ");
        uart_poll_puts(uint_to_dec_str(order));
        uart_poll_puts(" at frame ");
        uart_poll_puts(uint_to_dec_str(frame_idx));
        uart_poll_puts(" and frame ");
        uart_poll_putln(uint_to_dec_str(buddy_idx));
    }

    return BUDDY_UNGROUP_SUCCESS;
}