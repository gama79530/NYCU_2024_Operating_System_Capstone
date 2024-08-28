#include "frame.h"
#include "list.h"
#include "mini_uart.h"
#include "util.h"
#include "string.h"
#include "memory.h"

#define _memory_base        (_metadata->memory_base)
#define _memory_boundary    (_metadata->memory_boundary)
#define _frame_order        (_metadata->frame_order)
#define _frame_num          (_metadata->frame_num)
#define _buddy_order_limit  (_metadata->buddy_order_limit)
#define _buddy_array        (_metadata->buddy_array)
#define _buddy_lists        (_metadata->buddy_lists)

#define BUDDY_STATE_PRESERVED   0x80
#define BUDDY_STATE_ALLOCATED   0x81
#define BUDDY_STATE_BUDDY       0x82
#define BUDDY_STATE_ERROR       0x83

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

static bool buddy_group(void *metadata, uint64_t *frame_idx_ptr, bool verbose);
static int64_t get_order_of_frame(void *metadata, uint64_t frame_idx);

void* buddy_system_init(void *memory_base, void *memory_boundary, uint8_t frame_order, uint8_t buddy_order_limit, startup_malloc_callback_t malloc_cb, startup_preserve_memory_callback_t preserve_cb){
    void *metadata = NULL;
    buddy_sys_metadata_t *_metadata = NULL;
    list_head_t *temp = NULL;
    
    /* initialize buddy system metadata */    
    metadata = malloc_cb(sizeof(buddy_sys_metadata_t));
    if(metadata == NULL){
        uart_putln("buddy_system_init: metadata allocation fail.");
        return NULL;
    }

    _metadata = (buddy_sys_metadata_t*)metadata;
    _memory_base = memory_base;
    _memory_boundary = memory_boundary;
    _frame_order = frame_order;
    _frame_num = ((uint64_t)_memory_boundary - (uint64_t)_memory_base) >> _frame_order;
    _buddy_order_limit = buddy_order_limit;
    _buddy_array = NULL;
    _buddy_lists = NULL;

    if(NULL == (_buddy_array = (int8_t*)malloc_cb(_frame_num * sizeof(int8_t)))){
        uart_putln("buddy_system_init: buddy_array allocation fail.");
        return NULL;    
    }
    for(uint32_t i = 0; i < _frame_num; i++){
        _buddy_array[i] = 0;
    }

    if(NULL == (temp = (list_head_t*)malloc_cb(_buddy_order_limit * sizeof(list_head_t)))){
        uart_putln("buddy_system_init: buddy_lists allocation fail.");
        return NULL;    
    }
    for(uint32_t i = 0; i < _buddy_order_limit; i++){
        temp[i].prev = temp[i].next = &temp[i];
    }

    /* preserve memory */
    preserve_cb(metadata);

    /* build buddy system */
    for(uint8_t buddy_order = 0; buddy_order < _buddy_order_limit; buddy_order++){
        for(uint64_t frame_idx = 0; frame_idx < _frame_num; frame_idx += (2 << buddy_order)){
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
    buddy_sys_metadata_t *_metadata = (buddy_sys_metadata_t*)metadata;
    uint64_t begin_idx, end_idx;
    
    if(NULL != _buddy_lists){
#if VERBOSE == true
        uart_puts("buddy_preserve_memory: preserve memory after build is not allowed.");
#endif
        return false;
    }

    /* preserve range check */
    if((uint64_t)memory_base < (uint64_t)_memory_base || (uint64_t)memory_boundary > (uint64_t)_memory_boundary){
#if VERBOSE == true
        uart_putln("buddy_preserve_memory: out of memory space");
#endif
        return false;
    }

    begin_idx = buddy_addr_to_frame_idx(metadata, memory_base);
    end_idx = buddy_addr_to_frame_idx(metadata, memory_boundary - 1);

    for(uint64_t i = begin_idx; i <= end_idx; i++){
        _buddy_array[i] = (int8_t)BUDDY_STATE_PRESERVED;
    }

#if VERBOSE == true
    if(NULL != msg){
        uart_puts(msg);
        uart_puts(" (range: 0x");
        uart_puts(long_to_hex_str((uint64_t)memory_base) + 8);
        uart_puts(" - 0x");
        uart_puts(long_to_hex_str((uint64_t)memory_boundary) + 8);
        uart_puts(", begin_idx = ");
        uart_puts(uint_to_dec_str(begin_idx));
        uart_puts(", end_idx = ");
        uart_puts(uint_to_dec_str(end_idx));
        uart_putln(")");
    }
#endif

    return true;
}

void buddy_show_layout(void *metadata){
    buddy_sys_metadata_t *_metadata = (buddy_sys_metadata_t*)metadata;
    uart_putln("");
    uart_putln("********** show_memory_info **********");
    uart_puts("memory_base\t: 0x");
    uart_putln(long_to_hex_str((uint64_t)_memory_base) + 8);

    uart_puts("memory_boundary\t: 0x");
    uart_putln(long_to_hex_str((uint64_t)_memory_boundary) + 8);

    uart_puts("frame_order\t: 2^" );
    uart_puts(uint_to_dec_str(_frame_order));
    uart_putln(" bytes");

    uart_puts("frame_num\t: " );
    uart_putln(uint_to_dec_str(_frame_num));
    uart_putln("");

    uint64_t frame_idx = 0;
    while(frame_idx < _frame_num){
        buddy_show_frame_state(metadata, frame_idx);

        if(_buddy_array[frame_idx] == (int8_t)BUDDY_STATE_ALLOCATED){
            frame_idx += (1 << get_order_of_frame(metadata, frame_idx));
        }else if(_buddy_array[frame_idx] >= 0){
            frame_idx += (1 << _buddy_array[frame_idx]);
        }else{
            frame_idx++;
        }

    }
    
    uart_putln("");
}

void buddy_show_frame_state(void *metadata, uint64_t frame_idx){
    buddy_sys_metadata_t *_metadata = (buddy_sys_metadata_t*)metadata;
    uart_puts("The state of frame ");
    uart_puts(uint_to_dec_str(frame_idx));
    uart_puts(" is ");

    if(_buddy_array[frame_idx] == (int8_t)BUDDY_STATE_ALLOCATED){
        uart_putln("STATE_ALLOCATED");
    }else if(_buddy_array[frame_idx] == (int8_t)BUDDY_STATE_BUDDY){
        uart_putln("BUDDY_STATE_BUDDY");
    }else if(_buddy_array[frame_idx] == (int8_t)BUDDY_STATE_PRESERVED){
        uart_putln("BUDDY_STATE_PRESERVED");
    }else{
        uart_puts("at order ");
        uart_putln(uint_to_dec_str(_buddy_array[frame_idx]));
    }
}

void* frame_alloc(void *metadata, uint8_t buddy_order){
    buddy_sys_metadata_t *_metadata = (buddy_sys_metadata_t*)metadata;
    void *ret_frame_addr = NULL;
    uint64_t ret_frame_idx;

    // over the buddy system's limit
    if(buddy_order >= _buddy_order_limit){
#if VERBOSE == true
        uart_puts("frame_alloc: request buddy_order (= ");
        uart_puts(uint_to_dec_str(buddy_order));
        uart_puts(") is greater than or equal to the limit (= ");
        uart_puts(uint_to_dec_str(_buddy_order_limit));
        uart_putln(")");
#endif
    // find contiguous frame directly
    }else if(!list_is_empty(&_buddy_lists[buddy_order])){
        list_head_t *node = _buddy_lists[buddy_order].next;
        list_remove(node);
        ret_frame_addr = (void*)node;
        ret_frame_idx = buddy_addr_to_frame_idx(metadata, ret_frame_addr);
        _buddy_array[ret_frame_idx] = (int8_t)BUDDY_STATE_ALLOCATED;

#if VERBOSE == true
        uart_puts("frame_alloc: find contiguous frame ");
        uart_puts(uint_to_dec_str(ret_frame_idx));
        uart_puts(" with order ");
        uart_puts(uint_to_dec_str(buddy_order));
        uart_putln(" directly");
#endif
    // try to split buddy group with greater order
    }else{
        ret_frame_addr = frame_alloc(metadata, buddy_order + 1);
        // the memory space is not exhausted    
        if(ret_frame_addr != NULL){
            ret_frame_idx = buddy_addr_to_frame_idx(metadata, ret_frame_addr);

            uint64_t buddy_frame_idx = ret_frame_idx ^ (1 << buddy_order);
            void *buddy_frame_addr = buddy_frame_idx_to_addr(metadata, buddy_frame_idx);

            _buddy_array[buddy_frame_idx] = (int8_t)buddy_order;
            list_add((list_head_t*)buddy_frame_addr, _buddy_lists[buddy_order].prev, &_buddy_lists[buddy_order]);

#if VERBOSE == true
        uart_puts("frame_alloc: find contiguous frame with order ");
        uart_puts(uint_to_dec_str(buddy_order));
        uart_puts(" by split frame with order ");
        uart_putln(uint_to_dec_str(buddy_order + 1));
#endif
        }
    }
    
    return ret_frame_addr;
}

void frame_free(void *metadata, void *ptr){
    buddy_sys_metadata_t *_metadata = (buddy_sys_metadata_t*)metadata;

    /* out of range */
    if((uint64_t)ptr < (uint64_t)_memory_base || (uint64_t)ptr >= (uint64_t)_memory_boundary){
        return;
    }

    uint64_t frame_idx = buddy_addr_to_frame_idx(metadata, ptr);
    ptr = buddy_frame_idx_to_addr(metadata, frame_idx);
    if(_buddy_array[frame_idx] != (int8_t)BUDDY_STATE_ALLOCATED){
        return;
    }

#if VERBOSE == true
    uart_puts("frame_free: return frame ");
    uart_putln(uint_to_dec_str(frame_idx));
#endif

    /* find buddy_order */
    int64_t buddy_order = get_order_of_frame(metadata, frame_idx);

    /* return into buddy system */
    _buddy_array[frame_idx] = (int8_t)buddy_order;
    list_add((list_head_t*)ptr, _buddy_lists[buddy_order].prev, &_buddy_lists[buddy_order]);
    while(buddy_group(metadata, &frame_idx, VERBOSE));
}

void* buddy_frame_idx_to_addr(void *metadata, uint64_t frame_idx){
    buddy_sys_metadata_t *_metadata = (buddy_sys_metadata_t*)metadata;

    return frame_idx < _frame_num ? _memory_base + (frame_idx << _frame_order) : NULL;
}

uint64_t buddy_addr_to_frame_idx(void *metadata, void *addr){
    buddy_sys_metadata_t *_metadata = (buddy_sys_metadata_t*)metadata;
    
    return ((uint64_t)addr >= (uint64_t)_memory_base && (uint64_t)addr < (uint64_t)_memory_boundary) ? 
           ((uint64_t)addr - (uint64_t)_memory_base) >> _frame_order : 0;
}

static bool buddy_group(void *metadata, uint64_t *frame_idx_ptr, bool verbose){
    buddy_sys_metadata_t *_metadata = (buddy_sys_metadata_t*)metadata;
    uint64_t frame_idx, buddy_idx;
    int32_t buddy_order;
    
    frame_idx = *frame_idx_ptr;
    buddy_order = _buddy_array[frame_idx];

    if(frame_idx >= _frame_num || buddy_order < 0 || buddy_order >= _buddy_order_limit - 1){
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
    _buddy_array[buddy_idx] = (int8_t)BUDDY_STATE_BUDDY;

    /* processing buddy_lists */
    if(NULL != _buddy_lists){
        list_head_t *frame_node = (list_head_t*)buddy_frame_idx_to_addr(metadata, frame_idx);
        list_head_t *buddy_node = (list_head_t*)buddy_frame_idx_to_addr(metadata, buddy_idx);

        list_remove(frame_node);
        list_remove(buddy_node);
        list_add(frame_node, _buddy_lists[buddy_order].prev, &_buddy_lists[buddy_order]);
    }

    *frame_idx_ptr = frame_idx;

    if(verbose){
        uart_puts("buddy_group: group frame ");
        uart_puts(uint_to_dec_str(frame_idx));
        uart_puts(" and frame ");
        uart_puts(uint_to_dec_str(buddy_idx));
        uart_puts(" to make new contiguous buddy with order ");
        uart_putln(uint_to_dec_str(buddy_order));
    }

    return true;
}

static int64_t get_order_of_frame(void *metadata, uint64_t frame_idx){
    buddy_sys_metadata_t *_metadata = (buddy_sys_metadata_t*)metadata;
    int64_t buddy_order = 0;
    uint64_t buddy_idx;

    if(frame_idx >= _frame_num || _buddy_array[frame_idx] == (int8_t)BUDDY_STATE_PRESERVED){
        return (int8_t)BUDDY_STATE_ERROR;
    }else if(_buddy_array[frame_idx] >= 0){
        buddy_order = _buddy_array[frame_idx];
    }else{
        buddy_idx = frame_idx ^ (1 << buddy_order);
        while(_buddy_array[buddy_idx] == (int8_t)BUDDY_STATE_BUDDY){
            if(frame_idx > buddy_idx){
                swap(frame_idx, buddy_idx);
            }
            buddy_idx = frame_idx ^ (1 << ++buddy_order);
        }
    }
    
    return buddy_order;
}