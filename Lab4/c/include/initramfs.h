#ifndef LAB4_C_INITRAMFS_H
#define LAB4_C_INITRAMFS_H

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

/* Set the memory range that contains the initramfs cpio archive. */
void initramfs_set_range(uintptr_t begin, uintptr_t end);

/* Use the default QEMU initramfs memory range from config.h. */
void initramfs_use_default_range(void);

/* Read /chosen linux,initrd-start/end from a devicetree. */
fdt_error_t initramfs_read_range_from_fdt(const fdt_t *fdt, uintptr_t *begin, uintptr_t *end);

/* Convert an initramfs_error_t value into a user-facing error string. */
const char *initramfs_error_string(initramfs_error_t error);

/* Return the current initramfs range start as a byte pointer. */
const uint8_t *initramfs_begin(void);

/* Parse the next cpio newc entry and advance cursor. */
initramfs_error_t initramfs_next(const uint8_t **cursor, initramfs_file_t *file);

/* Match a shell query path against an archive entry name. */
bool initramfs_path_matches(const char *query, const char *archive_name);

#endif
