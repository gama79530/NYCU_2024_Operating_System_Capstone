#include "cpio.h"
#include "string.h"
#include "dtb.h"
#include "util.h"
#include "mini_uart.h"

/* "_cpio_ptr" is set by the default setting in "config.txt", but we can read the value 
   from the device tree rather than hard coding it. 
*/
static void *cpio_begin_ptr = (void*)0x8000000;
static void *cpio_end_ptr = (void*)0x08003600;

void* get_cpio_begin_ptr(){
    return cpio_begin_ptr;
}

void *get_cpio_end_ptr(void){
    return cpio_end_ptr;
}

int cpio_iter(void IN OUT **current_ref, file_info_t OUT *info_ref){
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
        info_ref->name_size = hex_str_to_uint(header->c_namesize);
    }

    // current is end of file
    if(!strcmp((char*)current + sizeof(cpio_newc_header_t), "TRAILER!!!")){
        *current_ref = 0;
        return CPIO_ITER_EOF;
    }

    // find offset for content
    offset = align_ceiling(sizeof(cpio_newc_header_t) + info_ref->name_size, 4);
    offset = sizeof(cpio_newc_header_t) + info_ref->name_size;
    if(offset % 4){
        offset += (4 - offset % 4);
    }

    // find content
    if(info_ref != NULL){
        info_ref->content = current + offset;
        info_ref->content_size = hex_str_to_uint(header->c_filesize);
    }

    // find offset for next
    offset = align_ceiling(offset + hex_str_to_uint(header->c_filesize), 4);
    *current_ref += offset;

    return 0;
}

void cpio_callback(uint32_t token, const char *name, const void *data, uint32_t len){
    if(FDT_PROP == to_little_u32(token) && !strcmp(name, "linux,initrd-start")){
        cpio_begin_ptr = (void*)(uint64_t)to_little_u32(*(uint32_t*)data);
    }else if(FDT_PROP == to_little_u32(token) && !strcmp(name, "linux,initrd-end")){
        cpio_end_ptr = (void*)(uint64_t)to_little_u32(*(uint32_t*)data);
    }
}

int cpio_init(void){
    int ret = fdt_traverse(cpio_callback);
    if(ret){
        uart_putln("cpio_init fail.");
    }else{
        uart_putln("cpio_init success.");
    }

    return ret;
}