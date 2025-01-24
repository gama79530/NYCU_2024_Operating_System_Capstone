#include "dtb.h"
#include "util.h"
#include "printf.h"
#include "string.h"
#include "config.h"

static void *dtb_ptr = NULL;
static void *dtb_mem_rsv_block_ptr = NULL;
static void *dtb_struct_block_ptr = NULL;
static void *dtb_string_block_ptr = NULL;
static uint32_t dtb_struct_block_size = 0;
static uint32_t dtb_totalsize = 0;

static int enter_new_node(void IN OUT **cursor_ptr, fdt_callback_t cb);

int dtb_init(void *dtb_ref){
    fdt_header_t *header = (fdt_header_t*)dtb_ref;
    uint32_t magic_little, off_mem_rsvmap_little, off_dt_struct_little, off_dt_strings_little;

    endian_exchange(&magic_little, &header->magic, 4);
    if(FDT_HEADER_MAGIC != magic_little){
        dtb_ptr = NULL;
        dtb_mem_rsv_block_ptr = NULL;
        dtb_struct_block_ptr = NULL;
        dtb_string_block_ptr = NULL;
        dtb_struct_block_size = 0;
        dtb_totalsize = 0;

#if VERBOSE != 0
        printf("***** dtb_init fail *****\n");
#endif

        return -1;
    }
    
    dtb_ptr = dtb_ref;
    endian_exchange(&off_mem_rsvmap_little, &header->off_mem_rsvmap, 4);
    dtb_mem_rsv_block_ptr = dtb_ptr + off_mem_rsvmap_little;
    endian_exchange(&off_dt_struct_little, &header->off_dt_struct, 4);
    dtb_struct_block_ptr = dtb_ptr + off_dt_struct_little;
    endian_exchange(&off_dt_strings_little, &header->off_dt_strings, 4);
    dtb_string_block_ptr = dtb_ptr + off_dt_strings_little;
    endian_exchange(&dtb_struct_block_size, &header->size_dt_struct, 4);
    endian_exchange(&dtb_totalsize, &header->totalsize, 4);

#if VERBOSE != 0
    printf("***** set_dtb_ptr success *****\n");
#endif

    return 0;
}

void* get_dtb_ptr(){
    return dtb_ptr;
}

uint32_t get_dtb_size(){
    return dtb_totalsize;
}

int fdt_traverse(fdt_callback_t cb){
    void *cursor = dtb_struct_block_ptr;
    uint32_t token, token_little;

    if(cursor == NULL)  return FDT_POINTER_ERROR;

    while(cursor < dtb_struct_block_ptr + dtb_struct_block_size){
        token = *(uint32_t*)cursor;
        endian_exchange(&token_little, &token, 4);
        switch(token_little){            
            case FDT_BEGIN_NODE:
                int ret = enter_new_node(&cursor, cb);
                if(ret != FDT_SUCCESS)  return ret;
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

    return FDT_SUCCESS;
}

static int enter_new_node(void IN OUT **cursor_ptr, fdt_callback_t cb){
    uint32_t token, token_little, nameoff_little, len_little;
    const char *node_name, *property_name;
    fdt_property_t *property;

    token = *(uint32_t*)*cursor_ptr;
    *cursor_ptr += 4;
    node_name = (const char*)(*cursor_ptr);
    cb(token, node_name, NULL, 0);

    *cursor_ptr += round_up(str_len(node_name) + 1, 4);
    while(*cursor_ptr < dtb_struct_block_ptr + dtb_struct_block_size){
        token = *(uint32_t*)*cursor_ptr;
        endian_exchange(&token_little, &token, 4);
        switch(token_little){
            case FDT_BEGIN_NODE:
                int ret = enter_new_node(cursor_ptr, cb);
                if(ret != FDT_SUCCESS)  return ret;
                break;
            case FDT_END_NODE:
                *cursor_ptr += 4;
                return FDT_SUCCESS;
            case FDT_PROP:
                *cursor_ptr += 4;

                property = (fdt_property_t*)*cursor_ptr;
                endian_exchange(&nameoff_little, &property->nameoff, 4);
                property_name = (const char*)(dtb_string_block_ptr + nameoff_little);
            
                *cursor_ptr += 8;
                cb(token, property_name, *cursor_ptr, property->len);

                endian_exchange(&len_little, &property->len, 4);
                *cursor_ptr += round_up(len_little, 4);
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
