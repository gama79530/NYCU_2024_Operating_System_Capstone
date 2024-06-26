#include "cpio.h"
#include "string.h"

static char *_cpio_ptr = (char*)0x8000000;

void set_cpio_ptr(char *cpio_ptr){
    _cpio_ptr = cpio_ptr;
}

char* get_cpio_ptr(){
    return _cpio_ptr;
}

int iter(char **current_ref, file_info_t *info_ref){
    char *current = *current_ref;
    unsigned long offset = 0;
    cpio_newc_header_t *header = (cpio_newc_header_t*)current;

    // magic number check
    if(strncmp(header->c_magic, "070701", 6)){
        *current_ref = 0;
        return 1;
    }

    // find name
    info_ref->name = current + sizeof(cpio_newc_header_t);
    info_ref->name_size = hex_str_to_uint(header->c_namesize);

    // current is end of file
    if(!strcmp(info_ref->name, "TRAILER!!!")){
        *current_ref = 0;
        return 2;
    }

    // find offset for content
    offset = sizeof(cpio_newc_header_t) + info_ref->name_size;
    if(offset % 4){
        offset += (4 - offset % 4);
    }

    // find content
    info_ref->content = current + offset;
    info_ref->content_size = hex_str_to_uint(header->c_filesize);

    // find offset for next
    offset += info_ref->content_size;
    if(offset % 4){
        offset += (4 - offset % 4);
    }
    *current_ref += offset;

    return 0;
}