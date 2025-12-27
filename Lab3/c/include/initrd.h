#ifndef LAB3_C_INITRD_H
#define LAB3_C_INITRD_H
#include "types.h"

/* CPIO archive - new ascii format:

+---------------------------------------------------------------+
| file 1:   header          cpio_newc_header_t                  |
|           file name       char *                              |
|           4-byte align    0 padding                           |
|           file content    char *                              |
|           4-byte align    0 padding                           |
+---------------------------------------------------------------+
| file 2:   header          cpio_newc_header_t                  |
|           file name       char *                              |
|           4-byte align    0 padding                           |
|           file content    char *                              |
|           4-byte align    0 padding                           |
+---------------------------------------------------------------+
|                                                               |
|                             .                                 |
|                             .                                 |
|                             .                                 |
|                                                               |
+---------------------------------------------------------------+
|                                                               |
|  The file with name "TRAILER!!!" indicates the end of files.  |
|                                                               |
+---------------------------------------------------------------+
*/

// cpio - New ASCII Format header
typedef struct {
    const char c_magic[6];  // "070701"
    const char c_ino[8];
    const char c_mode[8];
    const char c_uid[8];
    const char c_gid[8];
    const char c_nlink[8];
    const char c_mtime[8];
    const char c_filesize[8];  // capital hex string
    const char c_devmajor[8];
    const char c_devminor[8];
    const char c_rdevmajor[8];
    const char c_rdevminor[8];
    const char c_namesize[8];  // capital hex string
    const char c_check[8];
} cpio_newc_header_t;

typedef struct file_info {
    const char *name;
    uint32_t name_size;
    const char *content;
    uint32_t content_size;
} file_info_t;

#define ITER_END 1
#define ERR_MAGIC -1
#define ERR_MAGIC_MSG "invalid cpio magic"

int set_initrd(void);
const char *iter_begin(void);
int iter_next(const char **cursor IN OUT, file_info_t *info OUT);

#endif