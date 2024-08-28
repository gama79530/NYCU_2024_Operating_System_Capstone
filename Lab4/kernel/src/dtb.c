#include "dtb.h"
#include "util.h"
#include "mini_uart.h"
#include "string.h"

static void *_dtb_ptr = NULL;
static void *_dtb_mem_rsv_block_ptr = NULL;
static void *_dtb_struct_block_ptr = NULL;
static void *_dtb_string_block_ptr = NULL;
static uint32_t _dtb_struct_block_size = 0;

void set_dtb_ptr(void *dtb_ptr){
    fdt_header_t *header = (fdt_header_t*) dtb_ptr;
    if(FDT_HEADER_MAGIC != to_little_u32(header->magic)){
        _dtb_ptr = NULL;
        _dtb_mem_rsv_block_ptr = NULL;
        _dtb_struct_block_ptr = NULL;
        _dtb_string_block_ptr = NULL;
        _dtb_struct_block_size = 0;
        uart_putln("set_dtb_ptr fail.");
    }else{
        _dtb_ptr = dtb_ptr;
        _dtb_mem_rsv_block_ptr = _dtb_ptr + to_little_u32(header->off_mem_rsvmap);
        _dtb_struct_block_ptr = _dtb_ptr + to_little_u32(header->off_dt_struct);
        _dtb_string_block_ptr = _dtb_ptr + to_little_u32(header->off_dt_strings);
        _dtb_struct_block_size = to_little_u32(header->size_dt_struct);
        uart_putln("set_dtb_ptr success.");
    }
}

void* get_dtb_ptr(){
    return _dtb_ptr;
}

uint32_t get_dtb_size(){
    return _dtb_struct_block_size;
}

static int _add_node(void IN OUT **cursor_ptr, fdt_callback_t cb){
    int ret = 0;
    uint32_t token;  
    const char *node_name;
    fdt_property_t *property;
    const char *property_name;

    token = *(uint32_t*)*cursor_ptr;

    *cursor_ptr += 4;
    node_name = (const char*)(*cursor_ptr);
    cb(token, node_name, NULL, 0);

    *cursor_ptr += align_ceiling(str_len(node_name) + 1, 4);
    while(*cursor_ptr < _dtb_struct_block_ptr + _dtb_struct_block_size){
        token = *(uint32_t*)*cursor_ptr;
        switch(to_little_u32(token)){
            case FDT_BEGIN_NODE:
                if((ret = _add_node(cursor_ptr, cb))){
                    return ret;
                }
                break;
            case FDT_END_NODE:
                *cursor_ptr += 4;
                return FDT_SUCCESS;
            case FDT_PROP:
                *cursor_ptr += 4;

                property = (fdt_property_t*)*cursor_ptr;
                property_name = (const char *)_dtb_string_block_ptr + to_little_u32(property->nameoff);

                *cursor_ptr += 8;
                cb(token, property_name, *cursor_ptr, property->len);

                *cursor_ptr += align_ceiling(to_little_u32(property->len), 4);
                break;
            case FDT_NOP:
                *cursor_ptr += 4;
                break;
            case FDT_END:
            default:
                return FDT_PARSE_ERROR;
        }
    }

    return FDT_PARSE_ERROR; 
}

int fdt_traverse(fdt_callback_t cb){
    int ret = 0;
    void *cursor = _dtb_struct_block_ptr;
    uint32_t token;
    
    if(cursor == NULL){
        return FDT_POINTER_ERROR;
    }

    while(cursor < _dtb_struct_block_ptr + _dtb_struct_block_size){
        token = *(uint32_t*)cursor;        
        switch(to_little_u32(token)){
            case FDT_BEGIN_NODE:
                if((ret = _add_node(&cursor, cb))){
                    return ret;
                }
                break;
            case FDT_END_NODE:
            case FDT_PROP:
                return FDT_PARSE_ERROR;
            case FDT_NOP:
            case FDT_END:
                cursor += 4;
                break;
            default:
                return FDT_PARSE_ERROR;
        }
    }

    return ret;
}
