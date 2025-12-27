#include "initrd.h"
#include "common.h"
#include "config.h"
#include "device_tree.h"
#include "printf.h"

/* "initrd_start" is set by the default setting in "config.txt", but we can read the value from the
 * device tree rather than hard coding it. */
static const char *initrd_start = (const char *) 0x8000000;
static const char *initrd_boundary = (const char *) 0x8200000;

void initrd_dtb_cb(const char *node_name,
                   const char *property_name,
                   const char *property_value,
                   uint32_t property_len);

int set_initrd(void)
{
    int ret = fdt_traverse(initrd_dtb_cb);
#if VERBOSE == true
    if (!ret) {
#ifdef PRINTF_LONG_SUPPORT
        printf("initrd start   : 0x%lx\n", (uint64_t) initrd_start);
        printf("initrd boundary: 0x%lx\n", (uint64_t) initrd_boundary);
#else
        printf("initrd start   : 0x%x\n", (uint32_t) initrd_start);
        printf("initrd boundary: 0x%x\n", (uint32_t) initrd_boundary);
#endif
    }
#endif
    return ret;
}

void initrd_dtb_cb(const char *node_name,
                   const char *property_name,
                   const char *property_value,
                   uint32_t property_len)
{
    if (!strcmp(node_name, "chosen")) {
        if (!strcmp(property_name, "linux,initrd-start")) {
            initrd_start = (const char *) (uint64_t) endian_swap32(*(uint32_t *) property_value);
        } else if (!strcmp(property_name, "linux,initrd-end")) {
            initrd_boundary = (const char *) (uint64_t) endian_swap32(*(uint32_t *) property_value);
        }
    }
}

const char *iter_begin(void)
{
    return initrd_start;
}

int iter_next(const char **cursor, file_info_t *info)
{
    char *current_file = (char *) *cursor;
    uint64_t offset = 0;
    cpio_newc_header_t *header = (cpio_newc_header_t *) current_file;

    // check magic
    if (strncmp(header->c_magic, "070701", 6) != 0) {
        return ERR_MAGIC;
    }

    // file name
    info->name = current_file + sizeof(cpio_newc_header_t);
    info->name_size = strtonum(header->c_namesize, 16, 8);

    // current file is end file of initrd
    if (!strcmp(info->name, "TRAILER!!!")) {
        *cursor = NULL;
        return ITER_END;
    }

    // calculate offset to find file content
    offset = round_up(sizeof(cpio_newc_header_t) + info->name_size, 4);  // 4-byte align

    // file content
    info->content = current_file + offset;
    info->content_size = strtonum(header->c_filesize, 16, 8);

    // calculate offset to next file
    offset = round_up(offset + info->content_size, 4);  // 4-byte align
    *cursor = current_file + offset;

    return 0;
}
