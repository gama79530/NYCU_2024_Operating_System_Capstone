#include "frame.h"
#include "list.h"
#include "mini_uart.h"
#include "util.h"
#include "string.h"

#define _memory_base        (((buddy_sys_metadata_t*)metadata)->memory_base)
#define _memory_boundary    (((buddy_sys_metadata_t*)metadata)->memory_boundary)
#define _frame_order        (((buddy_sys_metadata_t*)metadata)->frame_order)
#define _frame_size         (((buddy_sys_metadata_t*)metadata)->frame_size)
#define _frame_num          (((buddy_sys_metadata_t*)metadata)->frame_num)
#define _buddy_order_limit  (((buddy_sys_metadata_t*)metadata)->buddy_order_limit)
#define _buddy_array        (((buddy_sys_metadata_t*)metadata)->buddy_array)
#define _buddy_lists        (((buddy_sys_metadata_t*)metadata)->buddy_lists)

#define BUDDY_STATE_PRESERVED     0x80
#define BUDDY_STATE_ALLOCATED     0x81
#define BUDDY_STATE_BUDDY         0x82

typedef struct buddy_sys_metadata{
    void        *memory_base;
    void        *memory_boundary;
    uint8_t     frame_order;
    uint64_t    frame_size;
    uint64_t    frame_num;
    uint8_t     buddy_order_limit;
    int8_t      *buddy_array;
    list_head_t *buddy_lists;
} buddy_sys_metadata_t;

static bool buddy_group(void *metadata, uint64_t *frame_idx_ptr, bool print_msg);
static bool buddy_ungroup(void *metadata, uint64_t frame_idx, bool print_msg);

void* buddy_system_init(void *memory_base, void *memory_boundary, uint8_t frame_order, uint8_t buddy_order_limit, startup_malloc_callback_t malloc_cb, startup_preserve_memory_callback_t preserve_cb){
    void *metadata = NULL;
    list_head_t *temp = NULL;

    /* initialize buddy system metadata */
    metadata = malloc_cb(sizeof(buddy_sys_metadata_t));
    if(metadata == NULL){
        uart_poll_putln("buddy_system_init: metadata allocation fail.");
        return NULL;
    }

    _memory_base = memory_base;
    _memory_boundary = memory_boundary;
    _frame_order = frame_order;
    _frame_size = (1L << frame_order);    
    _frame_num = ((uint64_t)_memory_boundary - (uint64_t)_memory_base) >> _frame_order;
    _buddy_order_limit = buddy_order_limit;
    _buddy_array = NULL;
    _buddy_lists = NULL;

    if(NULL == (_buddy_array = (int8_t*)malloc_cb(_frame_num * sizeof(int8_t)))){
        uart_poll_putln("buddy_system_init: buddy_array allocation fail.");
        return NULL;    
    }
    if(NULL == (temp = (list_head_t*)malloc_cb(_buddy_order_limit * sizeof(list_head_t)))){
        uart_poll_putln("buddy_system_init: buddy_lists allocation fail.");
        return NULL;    
    }
    for(uint32_t i = 0; i < _buddy_order_limit; i++){
        temp[i].prev = temp[i].next = &temp[i];
    }

    /* preserve memory */
    preserve_cb(metadata);

    /* build buddy system */
    for(uint8_t buddy_order = 0; buddy_order < _buddy_order_limit; buddy_order++){
        for(uint64_t frame_idx = 0; frame_idx < _frame_num; frame_idx += (1 << buddy_order)){
            buddy_group(metadata, &frame_idx, false);
        }
    }

    _buddy_lists = temp;
    int32_t buddy_order;
    list_head_t *node;
    for(uint64_t frame_idx = 0; frame_idx < _frame_num; frame_idx++){
        buddy_order = _buddy_array[frame_idx];
        /* this frame is not a head of buddy groups */
        if(buddy_order < 0) continue;
        node = (list_head_t*)buddy_frame_idx_to_addr(metadata, frame_idx);
        list_add(node, _buddy_lists[buddy_order].prev, &_buddy_lists[buddy_order]);
    }
    return metadata;
}

bool buddy_preserve_memory(void *metadata, void *memory_base, void *memory_boundary, char *msg){
    uint64_t begin_idx, end_idx;
    
    if(NULL != _buddy_lists){
        uart_poll_puts("buddy_preserve_memory: preserve memory after build is not allowed.");
        return false;
    }

    /* preserve range check */
    if((uint64_t)memory_base < (uint64_t)_memory_base || (uint64_t)memory_boundary > (uint64_t)_memory_boundary){
        uart_poll_puts("buddy_preserve_memory: out of memory space");
        return false;
    }

    begin_idx = buddy_addr_to_frame_idx(metadata, (void*)align_floor((uint64_t)memory_base, _frame_size));
    end_idx = buddy_addr_to_frame_idx(metadata, (void*)align_floor((uint64_t)(memory_boundary - 1), _frame_size));

    for(uint64_t i = begin_idx; i <= end_idx; i++){
        _buddy_array[i] = BUDDY_STATE_PRESERVED;
    }

    if(NULL != msg){
        uart_poll_puts(msg);
        uart_poll_puts(" (range: 0x");
        uart_poll_puts(long_to_hex_str((uint64_t)memory_base) + 8);
        uart_poll_puts(" - 0x");
        uart_poll_puts(long_to_hex_str((uint64_t)memory_boundary) + 8);
        uart_poll_puts(", begin_idx = ");
        uart_poll_puts(uint_to_dec_str(begin_idx));
        uart_poll_puts(", end_idx = ");
        uart_poll_puts(uint_to_dec_str(end_idx));
        uart_poll_putln(")");
    }

    return true;
}

void buddy_show_layout(void *metadata){
    uart_poll_putln("");
    uart_poll_putln("********** show_memory_info **********");
    uart_poll_puts("memory_base\t: 0x");
    uart_poll_putln(long_to_hex_str((uint64_t)_memory_base) + 8);

    uart_poll_puts("memory_boundary\t: 0x");
    uart_poll_putln(long_to_hex_str((uint64_t)_memory_boundary) + 8);

    uart_poll_puts("frame_order\t: 2^" );
    uart_poll_puts(uint_to_dec_str(_frame_order));
    uart_poll_putln(" bytes");

    uart_poll_puts("frame_num\t: " );
    uart_poll_putln(uint_to_dec_str(_frame_num));
    uart_poll_putln("");

    uint64_t frame_idx = 0;
    while(frame_idx < _frame_num){
        buddy_show_frame_state(metadata, frame_idx);
        frame_idx += (_buddy_array[frame_idx] >= 0 ? 1 << _buddy_array[frame_idx] : 1);
    }
    
    uart_poll_putln("");
}

void buddy_show_frame_state(void *metadata, uint64_t frame_idx){
    uart_poll_puts("The state of frame ");
    uart_poll_puts(uint_to_dec_str(frame_idx));
    uart_poll_puts(" is ");

    if((int8_t)BUDDY_STATE_ALLOCATED == _buddy_array[frame_idx]){
        uart_poll_putln("STATE_ALLOCATED");
    }else if((int8_t)BUDDY_STATE_BUDDY == _buddy_array[frame_idx]){
        uart_poll_putln("BUDDY_STATE_BUDDY");
    }else if((int8_t)BUDDY_STATE_PRESERVED == _buddy_array[frame_idx]){
        uart_poll_putln("BUDDY_STATE_PRESERVED");
    }else{
        uart_poll_puts("at order ");
        uart_poll_putln(uint_to_dec_str(_buddy_array[frame_idx]));
    }
}

void* buddy_frame_idx_to_addr(void *metadata, uint64_t frame_idx){
    return _memory_base + (frame_idx << _frame_order);
}

uint64_t buddy_addr_to_frame_idx(void *metadata, void *addr){
    return ((uint64_t)addr - (uint64_t)_memory_base) >> _frame_order;
}

static bool buddy_group(void *metadata, uint64_t *frame_idx_ptr, bool print_msg){
    uint64_t frame_idx, buddy_idx;
    int32_t buddy_order;
    
    frame_idx = *frame_idx_ptr;
    buddy_order = _buddy_array[frame_idx];

    if(buddy_order < 0 || buddy_order >= _buddy_order_limit - 1){
        return false;
    }

    buddy_idx = frame_idx ^ (1 << buddy_order);
    if(frame_idx > buddy_idx){
        swap(frame_idx, buddy_idx);
    }

    if(_buddy_array[frame_idx] != _buddy_array[buddy_idx]){
        return false;
    }

    _buddy_array[frame_idx] = ++buddy_order;
    _buddy_array[buddy_idx] = BUDDY_STATE_BUDDY;

    /* processing buddy_lists */
    if(NULL != _buddy_lists){
        list_head_t *frame_node = (list_head_t*)buddy_frame_idx_to_addr(metadata, frame_idx);
        list_head_t *buddy_node = (list_head_t*)buddy_frame_idx_to_addr(metadata, buddy_idx);

        list_remove(frame_node);
        list_remove(buddy_node);
        list_add(frame_node, _buddy_lists[buddy_order].prev, &_buddy_lists[buddy_order]);
    }

    *frame_idx_ptr = frame_idx;

    if(print_msg){
        uart_poll_puts("group frame ");
        uart_poll_puts(uint_to_dec_str(frame_idx));
        uart_poll_puts(" and frame ");
        uart_poll_puts(uint_to_dec_str(buddy_idx));
        uart_poll_puts(" to make new contiguous buddy with order ");
        uart_poll_putln(uint_to_dec_str(buddy_order));
    }

    return true;
}

static bool buddy_ungroup(void *metadata, uint64_t frame_idx, bool print_msg){
    int32_t buddy_order = _buddy_array[frame_idx] - 1;

    if(buddy_order < 0){
        return false;
    }

    uint64_t buddy_idx = frame_idx ^ (1 << buddy_order);
    _buddy_array[frame_idx] = _buddy_array[buddy_idx] = buddy_order;

    /* processing buddy_lists */
    list_head_t *frame_node = (list_head_t*)buddy_frame_idx_to_addr(metadata, frame_idx);
    list_head_t *buddy_node = (list_head_t*)buddy_frame_idx_to_addr(metadata, buddy_idx);

    list_remove(frame_node);
    list_add(frame_node, _buddy_lists[buddy_order].prev, &_buddy_lists[buddy_order]);
    list_add(buddy_node, _buddy_lists[buddy_order].prev, &_buddy_lists[buddy_order]);

    if(print_msg){
        uart_poll_puts("ungroup to make two new contiguous buddy with order ");
        uart_poll_puts(uint_to_dec_str(buddy_order));
        uart_poll_puts(" at frame ");
        uart_poll_puts(uint_to_dec_str(frame_idx));
        uart_poll_puts(" and frame ");
        uart_poll_putln(uint_to_dec_str(buddy_idx));
    }

    return true;
}

