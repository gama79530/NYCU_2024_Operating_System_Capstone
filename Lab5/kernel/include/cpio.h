#ifndef __CPIO_H__
#define __CPIO_H__
#include "types.h"

/*  
CPIO archive - new ascii format:
ref: https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5

+-------------------------------------------------------------------+
| file 1:   header          cpio_newc_header_t                      |
|           file name       char *                                  |
|           4-byte align    0 padding                               |
|           file content    char *                                  |
|           4-byte align    0 padding                               |
+-------------------------------------------------------------------+
| file 2:   header          cpio_newc_header_t                      |
|           file name       char *                                  |
|           4-byte align    0 padding                               |
|           file content    char *                                  |
|           4-byte align    0 padding                               |
+-------------------------------------------------------------------+
|                                                                   |
|                               .                                   |
|                               .                                   |
|                               .                                   |
|                                                                   |
+-------------------------------------------------------------------+
|                                                                   |
|  The file with name "TRAILER!!!" indicates the end of initramfs.  |
|                                                                   |
+-------------------------------------------------------------------+
*/

// cpio - New ASCII Format header
typedef struct{
    char c_magic[6];        // "070701"
    char c_ino[8];
    char c_mode[8];
    char c_uid[8];
    char c_gid[8];
    char c_nlink[8];
    char c_mtime[8];
    char c_filesize[8];     // capital hex string
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8];     // capital hex string
    char c_check[8];
} cpio_newc_header_t;

typedef struct file_info{
    char *name;
    uint32_t name_size;
    void *content;
    uint32_t content_size;
} file_info_t;

void* get_cpio_base(void);
void* get_cpio_boundary(void);

#define CPIO_SUCCESS            0
#define CPIO_ITER_EOF           1
#define CPIO_ITER_MAGIC_ERROR   -1

/*  coding template

void *cursor = get_cpio_base();
file_info_t info;
int iter_ret = 0;
while(true){
    iter_ret = cpio_file_iter(&cursor, &info);  // extract file info
    if(iter_ret == CPIO_ITER_EOF){
        break;
    }else if(iter_ret != CPIO_SUCCESS){
        // error handling
        break;
    }else{
        // using "strcmp" and info.name to process corresponding file 
    }
}
*/
int cpio_file_iter(void IN OUT **current_ref, file_info_t OUT *info_ref);

void cpio_set_by_dtb_callback(uint32_t token, const char *name, const void *data, uint32_t len);

int cpio_init(void);

#endif