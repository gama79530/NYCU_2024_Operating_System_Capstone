#ifndef LAB2_C_FDT_H
#define LAB2_C_FDT_H

#include "types.h"

typedef enum {
    FDT_SUCCESS = 0,
    FDT_ERROR_INVALID_ARGUMENT = -1,
    FDT_ERROR_INVALID_MAGIC = -2,
    FDT_ERROR_OUT_OF_RANGE = -3,
    FDT_ERROR_INVALID_STRUCTURE = -4,
    FDT_ERROR_NOT_FOUND = -5,
} fdt_error_t;

typedef struct {
    const uint8_t *base;
    size_t total_size;
    const uint8_t *struct_begin;
    const uint8_t *struct_end;
    const char *strings_begin;
    const char *strings_end;
} fdt_t;

const char *fdt_error_string(fdt_error_t error);
fdt_error_t fdt_init(fdt_t *fdt, uintptr_t address);
fdt_error_t fdt_get_property(const fdt_t *fdt,
                             const char *path,
                             const char *name,
                             const uint8_t **value,
                             size_t *size);

#endif
