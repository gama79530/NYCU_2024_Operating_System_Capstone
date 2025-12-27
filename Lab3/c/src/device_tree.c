#include "device_tree.h"
#include "common.h"
#include "config.h"
#if VERBOSE == true
#include "printf.h"
#endif


const char *fdt_err_msg[] = {"Invalid device tree header magic.",
                             "Invalid device tree header pointer.",
                             "Error occurred when parsing device tree structure block."};

static const char *dtb_header = NULL;
static const char *dtb_mem_rsv_block = NULL;
static const char *dtb_struct_block = NULL;
static const char *dtb_string_block = NULL;
static uint32_t dtb_struct_block_size = 0;

static int enter_node(const char **cursor_ptr, fdt_callback_t cb);
static int enter_property(const char **cursor_ptr, const char *node_name, fdt_callback_t cb);

int set_dtb_address(uint64_t addr)
{
    fdt_header_t *header = (fdt_header_t *) addr;
    if (FDT_HEADER_MAGIC != endian_swap32(header->magic)) {
        dtb_header = NULL;
        dtb_mem_rsv_block = NULL;
        dtb_struct_block = NULL;
        dtb_string_block = NULL;
        dtb_struct_block_size = 0;
        return FDT_HEADER_MAGIC_ERROR;
    }

#if VERBOSE == true
#ifdef PRINTF_LONG_SUPPORT
    printf("Device Tree Blob Address: 0x%lx\n", addr);
#else
    printf("Device Tree Blob Address: 0x%x\n", (uint32_t) addr);
#endif
#endif
    dtb_header = (const char *) addr;
    dtb_mem_rsv_block = (const char *) addr + endian_swap32(header->off_mem_rsvmap);
    dtb_struct_block = (const char *) addr + endian_swap32(header->off_dt_struct);
    dtb_string_block = (const char *) addr + endian_swap32(header->off_dt_strings);
    dtb_struct_block_size = endian_swap32(header->size_dt_struct);

    return 0;
}

fdt_header_t *get_dtb_info(void)
{
    return (fdt_header_t *) dtb_header;
}

int fdt_traverse(fdt_callback_t cb)
{
    if (dtb_struct_block == NULL) {
        return FDT_HEADER_POINTER_ERROR;
    }

    const char *cursor = dtb_struct_block;
    while (cursor < dtb_struct_block + dtb_struct_block_size) {
        uint32_t token = endian_swap32(*(uint32_t *) cursor);
        switch (token) {
        case FDT_BEGIN_NODE:
            if (enter_node(&cursor, cb)) {
                return FDT_PARSE_ERROR;
            }
            break;
        case FDT_END_NODE:
            return FDT_PARSE_ERROR;
        case FDT_PROP:
            return FDT_PARSE_ERROR;
        case FDT_NOP:
            cursor += 4;
            break;
        case FDT_END:
            return 0;
        default:
            return FDT_PARSE_ERROR;
        }
    }

    return 0;
}

static int enter_node(const char **cursor_ptr, fdt_callback_t cb)
{
    bool reach_node_end = false;
    *cursor_ptr += 4;  // skip FDT_BEGIN_NODE token

    const char *node_name = (const char *) (*cursor_ptr);  // extract node name
    int node_name_len = strlen(node_name);
    *cursor_ptr += round_up(node_name_len + 1, 4);  // move cursor to next 4-byte aligned address

    while (!reach_node_end) {
        uint32_t token = endian_swap32(*(uint32_t *) (*cursor_ptr));
        switch (token) {
        case FDT_BEGIN_NODE:
            if (enter_node(cursor_ptr, cb)) {
                return FDT_PARSE_ERROR;
            }
            break;
        case FDT_END_NODE:
            reach_node_end = true;
            *cursor_ptr += 4;  // skip FDT_END_NODE token
            break;
        case FDT_PROP:
            if (enter_property(cursor_ptr, node_name, cb)) {
                return FDT_PARSE_ERROR;
            }
            break;
        case FDT_NOP:
            *cursor_ptr += 4;  // skip FDT_NOP token
            break;
        case FDT_END:
            return FDT_PARSE_ERROR;
        default:
            return FDT_PARSE_ERROR;
        }
    }

    return 0;
}

static int enter_property(const char **cursor_ptr, const char *node_name, fdt_callback_t cb)
{
    *cursor_ptr += 4;  // skip FDT_PROP token
    fdt_property_t *prop = (fdt_property_t *) (*cursor_ptr);
    uint32_t prop_len = endian_swap32(prop->len);
    uint32_t prop_nameoff = endian_swap32(prop->nameoff);
    const char *prop_name = (const char *) dtb_string_block + prop_nameoff;
    const char *prop_value = (const char *) (*cursor_ptr + sizeof(fdt_property_t));
    cb(node_name, prop_name, prop_value, prop_len);
    *cursor_ptr += sizeof(fdt_property_t) +
                   round_up(prop_len, 4);  // move cursor to next 4-byte aligned address

    return 0;
}