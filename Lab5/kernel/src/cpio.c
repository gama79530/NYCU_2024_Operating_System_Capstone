#include "cpio.h"
#include "string.h"
#include "util.h"
#include "dtb.h"
#include "config.h"
#include "printf.h"

/* "cpio_base" is set by the default setting in "config.txt", 
    but we can read the value from the device tree rather than hard coding it. 
*/
static void *cpio_base = (void*)0x8000000;
static void *cpio_boundary = (void*)0x08003600;

void* get_cpio_base(void){
    return cpio_base;
}

void* get_cpio_boundary(void){
    return cpio_boundary;
}

int cpio_file_iter(void IN OUT **current_ref, file_info_t OUT *info_ref){
    void *current = *current_ref;
    uint64_t offset = 0;
    cpio_newc_header_t *header = (cpio_newc_header_t*)current;

    // magic number check
    if(strncmp(header->c_magic, "070701", 6)){
        *current_ref = 0;
        return CPIO_ITER_MAGIC_ERROR;
    }

    // find file name
    if(info_ref != NULL){
        info_ref->name = (char*)current + sizeof(cpio_newc_header_t);
        info_ref->name_size = (uint32_t)hex_str_to_uint(header->c_namesize, 8);
    }

    // current file is end of file
    if(!strcmp((char*)current + sizeof(cpio_newc_header_t), "TRAILER!!!")){
        *current_ref = 0;
        return CPIO_ITER_EOF;
    }

    // find offset for content
    offset = round_up(sizeof(cpio_newc_header_t) + info_ref->name_size, 4);

    // find content
    if(info_ref != NULL){
        info_ref->content = current + offset;
        info_ref->content_size = hex_str_to_uint(header->c_filesize, 8);
    }

    // find offset for next
    offset = round_up(offset + hex_str_to_uint(header->c_filesize, 8), 4);
    *current_ref += offset;

    return 0;
}

void cpio_set_by_dtb_callback(uint32_t token, const char *name, const void *data, uint32_t len){
    uint32_t token_little, data_little;
    endian_exchange(&token, &token_little, 4);

    if(token_little == FDT_PROP){
        if(!strcmp(name, "linux,initrd-start")){
            endian_exchange((uint32_t*)data, &data_little, 4);
            cpio_base = (void*)(uint64_t)data_little;
        }else if(!strcmp(name, "linux,initrd-end")){
            endian_exchange((uint32_t*)data, &data_little, 4);
            cpio_boundary = (void*)(uint64_t)data_little;
        }
    }
}


int cpio_init(void){
    if(fdt_traverse(cpio_set_by_dtb_callback) != FDT_SUCCESS){
#if VERBOSE != 0
        printf("***** cpio_init fail *****\n");
#endif
        return -1;
    }

#if VERBOSE != 0
    printf("***** cpio_init success *****\n");
#endif

    return 0;
}
