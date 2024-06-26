#ifndef CPIO_H
#define CPIO_H

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
typedef struct{
    char c_magic[6];
    char c_ino[8];
    char c_mode[8];
    char c_uid[8];
    char c_gid[8];
    char c_nlink[8];
    char c_mtime[8];
    char c_filesize[8]; // capital hex string
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8]; // capital hex string
    char c_check[8];
} cpio_newc_header_t;


typedef struct file_info{
    char *name;
    unsigned int name_size;
    char *content;
    unsigned int content_size;
} file_info_t;


void set_cpio_ptr(char *cpio_ptr);
char* get_cpio_ptr(void);

int iter(char **current_ref, file_info_t *info_ref);

#endif