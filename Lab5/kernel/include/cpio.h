#ifndef __CPIO_H__
#define __CPIO_H__
#include "types.h"

/* CPIO archive - new ascii format:

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
    unsigned int name_size;
    void *content;
    unsigned int content_size;
} file_info_t;

void* get_cpio_base(void);
void* get_cpio_boundary(void);

#define CPIO_SUCCESS            0
#define CPIO_ITER_EOF           1
#define CPIO_ITER_MAGIC_ERROR   -1

int cpio_file_iter(void IN OUT **current_ref, file_info_t OUT *info_ref);

void cpio_set_by_dtb_callback(uint32_t token, const char *name, const void *data, uint32_t len);

int cpio_init(void);

#endif