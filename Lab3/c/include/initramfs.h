#ifndef LAB3_C_INITRAMFS_H
#define LAB3_C_INITRAMFS_H

#include "fdt.h"
#include "types.h"

typedef enum {
    INITRAMFS_SUCCESS = 0,
    INITRAMFS_END = 1,
    INITRAMFS_ERROR_INVALID_ARGUMENT = -1,
    INITRAMFS_ERROR_INVALID_MAGIC = -2,
    INITRAMFS_ERROR_INVALID_FORMAT = -3,
    INITRAMFS_ERROR_OUT_OF_RANGE = -4,
} initramfs_error_t;

typedef struct {
    const char *name;
    size_t name_size;
    const uint8_t *data;
    size_t size;
} initramfs_file_t;

void initramfs_set_range(uintptr_t begin, uintptr_t end);
void initramfs_use_default_range(void);
fdt_error_t initramfs_read_range_from_fdt(const fdt_t *fdt, uintptr_t *begin, uintptr_t *end);
const char *initramfs_error_string(initramfs_error_t error);
const uint8_t *initramfs_begin(void);
initramfs_error_t initramfs_next(const uint8_t **cursor, initramfs_file_t *file);
bool initramfs_path_matches(const char *query, const char *archive_name);

#endif
