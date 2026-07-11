#include "initramfs.h"

#include "config.h"
#include "error.h"
#include "string.h"
#include "util.h"

#define CPIO_NEWC_MAGIC "070701"
#define CPIO_NEWC_HEADER_SIZE 110
#define CPIO_NEWC_FIELD_SIZE 8
#define CPIO_NEWC_FILESIZE_OFFSET 54
#define CPIO_NEWC_NAMESIZE_OFFSET 94
#define CPIO_NEWC_ALIGNMENT 4

/*
 * CPIO newc entry layout:
 *
 * +--------------------------------------------------+
 * | header: 110-byte ASCII fields                    |
 * | name: c_namesize bytes, including trailing '\0'  |
 * | padding: align next field to 4 bytes             |
 * | data: c_filesize bytes                           |
 * | padding: align next entry to 4 bytes             |
 * +--------------------------------------------------+
 *
 * Numeric header fields are fixed-width ASCII hex strings.
 * The archive ends at an entry named "TRAILER!!!".
 */

/* Private function declarations */
static bool range_contains(uintptr_t begin, uintptr_t size);
static bool read_hex_field(const uint8_t *cursor, size_t offset, size_t *value);

/* Private data */
static uintptr_t initramfs_range_begin;
static uintptr_t initramfs_range_end;

static const char *const initramfs_error_messages[] = {
    "Invalid argument.",
    "Invalid cpio magic.",
    "Invalid cpio format.",
    "Initramfs entry is out of range.",
};

#define INITRAMFS_ERROR_COUNT \
    (sizeof(initramfs_error_messages) / sizeof(initramfs_error_messages[0]))

/* Function implementations */
void initramfs_set_range(uintptr_t begin, uintptr_t end)
{
    initramfs_range_begin = begin;
    initramfs_range_end = end;
}

void initramfs_use_default_range(void)
{
    initramfs_set_range(CONFIG_INITRAMFS_BASE, CONFIG_INITRAMFS_END);
}

fdt_error_t initramfs_read_range_from_fdt(const fdt_t *fdt, uintptr_t *begin, uintptr_t *end)
{
    const uint8_t *value;
    size_t size;
    uintptr_t initrd_begin;
    uintptr_t initrd_end;

    if (fdt == NULL || begin == NULL || end == NULL) {
        return FDT_ERROR_INVALID_ARGUMENT;
    }

    if (fdt_get_property(fdt, "/chosen", "linux,initrd-start", &value, &size) !=
            FDT_SUCCESS ||
        size < sizeof(uint32_t)) {
        return FDT_ERROR_NOT_FOUND;
    }
    initrd_begin = (uintptr_t) read_be32(value);

    if (fdt_get_property(fdt, "/chosen", "linux,initrd-end", &value, &size) != FDT_SUCCESS ||
        size < sizeof(uint32_t)) {
        return FDT_ERROR_NOT_FOUND;
    }
    initrd_end = (uintptr_t) read_be32(value);

    if (initrd_begin == 0 || initrd_end == 0 || initrd_end <= initrd_begin) {
        return FDT_ERROR_NOT_FOUND;
    }

    *begin = initrd_begin;
    *end = initrd_end;
    return FDT_SUCCESS;
}

const char *initramfs_error_string(initramfs_error_t error)
{
    if (error == INITRAMFS_END) {
        return "End of archive.";
    }

    if (error >= INITRAMFS_SUCCESS) {
        return "Unknown initramfs error.";
    }

    size_t index = error_code_to_index(error);

    if (index >= INITRAMFS_ERROR_COUNT) {
        return "Unknown initramfs error.";
    }

    return initramfs_error_messages[index];
}

const uint8_t *initramfs_begin(void)
{
    return (const uint8_t *) initramfs_range_begin;
}

initramfs_error_t initramfs_next(const uint8_t **cursor, initramfs_file_t *file)
{
    const uint8_t *entry;
    size_t name_size;
    size_t file_size;
    uintptr_t name_begin;
    uintptr_t name_end;
    uintptr_t data_begin;
    uintptr_t data_end;
    uintptr_t next_entry;

    if (cursor == NULL || *cursor == NULL || file == NULL) {
        return INITRAMFS_ERROR_INVALID_ARGUMENT;
    }

    entry = *cursor;
    if (!range_contains((uintptr_t) entry, CPIO_NEWC_HEADER_SIZE)) {
        return INITRAMFS_ERROR_OUT_OF_RANGE;
    }

    if (strncmp((const char *) entry, CPIO_NEWC_MAGIC, sizeof(CPIO_NEWC_MAGIC) - 1) != 0) {
        return INITRAMFS_ERROR_INVALID_MAGIC;
    }

    if (!read_hex_field(entry, CPIO_NEWC_NAMESIZE_OFFSET, &name_size) ||
        !read_hex_field(entry, CPIO_NEWC_FILESIZE_OFFSET, &file_size)) {
        return INITRAMFS_ERROR_INVALID_FORMAT;
    }

    name_begin = (uintptr_t) entry + CPIO_NEWC_HEADER_SIZE;
    name_end = name_begin + name_size;
    if (!range_contains(name_begin, name_size)) {
        return INITRAMFS_ERROR_OUT_OF_RANGE;
    }
    if (name_size == 0 || ((const char *) name_begin)[name_size - 1] != '\0') {
        return INITRAMFS_ERROR_INVALID_FORMAT;
    }

    file->name = (const char *) name_begin;
    file->name_size = name_size;
    file->data = NULL;
    file->size = 0;

    if (strcmp("TRAILER!!!", file->name) == 0) {
        *cursor = NULL;
        return INITRAMFS_END;
    }

    data_begin = align_up(name_end, CPIO_NEWC_ALIGNMENT);
    data_end = data_begin + file_size;
    if (!range_contains(data_begin, file_size)) {
        return INITRAMFS_ERROR_OUT_OF_RANGE;
    }

    next_entry = align_up(data_end, CPIO_NEWC_ALIGNMENT);
    if (next_entry > initramfs_range_end) {
        return INITRAMFS_ERROR_OUT_OF_RANGE;
    }

    file->data = (const uint8_t *) data_begin;
    file->size = file_size;
    *cursor = (const uint8_t *) next_entry;

    return INITRAMFS_SUCCESS;
}

bool initramfs_path_matches(const char *query, const char *archive_name)
{
    if (strcmp(query, archive_name) == 0) {
        return true;
    }

    if (archive_name[0] == '.' && archive_name[1] == '/') {
        return strcmp(query, archive_name + 2) == 0;
    }

    if (query[0] == '.' && query[1] == '/') {
        return strcmp(query + 2, archive_name) == 0;
    }

    return false;
}

static bool range_contains(uintptr_t begin, uintptr_t size)
{
    uintptr_t end = begin + size;

    if (end < begin) {
        return false;
    }

    return begin >= initramfs_range_begin && end <= initramfs_range_end;
}

static bool read_hex_field(const uint8_t *cursor, size_t offset, size_t *value)
{
    unsigned long result;

    if (!strntoul((const char *) cursor + offset, CPIO_NEWC_FIELD_SIZE, 16, &result)) {
        return false;
    }

    *value = (size_t) result;
    return true;
}
