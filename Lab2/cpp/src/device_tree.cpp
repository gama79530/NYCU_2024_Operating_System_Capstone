#include "device_tree.hpp"
#include "common.hpp"
#include "config.hpp"
#if VERBOSE == true
#include "kernel.hpp"
#include "printf.hpp"
#endif

const char *const DeviceTree::errMessages[] = {
    "Invalid device tree header magic.", "Invalid device tree header pointer.",
    "Error occurred when parsing device tree structure block."};

int DeviceTree::init(uint64_t addr)
{
    header = (FdtHeader *) addr;
    initialized = FDT_HEADER_MAGIC == util::endian_swap(header->magic);
    if (!initialized) {
        dtbHeader = nullptr;
        dtbMemRsvBlock = nullptr;
        dtbStructBlock = nullptr;
        dtbStringBlock = nullptr;
        dtbStructBlockSize = 0;
        header = nullptr;
        return FDT_HEADER_MAGIC_ERROR;
    }

#if VERBOSE == true
#ifdef PRINTF_LONG_SUPPORT
    kernel->printf("Device Tree Blob Address: 0x%lx\n", addr);
#else
    kernel->printf("Device Tree Blob Address: 0x%x\n", (uint32_t) addr);
#endif
#endif
    dtbHeader = (const char *) addr;
    dtbMemRsvBlock = (const char *) addr + util::endian_swap(header->offMemRsvmap);
    dtbStructBlock = (const char *) addr + util::endian_swap(header->offDtStruct);
    dtbStringBlock = (const char *) addr + util::endian_swap(header->offDtStrings);
    dtbStructBlockSize = util::endian_swap(header->sizeDtStruct);

    return FDT_SUCCESS;
}

DeviceTree::FdtHeader *DeviceTree::getHeader() const
{
    return header;
}

DeviceTree::ReturnCode DeviceTree::traverse(TraverseCallback callback)
{
    if (!initialized) {
        return FDT_HEADER_POINTER_ERROR;
    }

    const char *cursor = dtbStructBlock;
    while (cursor < dtbStructBlock + dtbStructBlockSize) {
        uint32_t token = util::endian_swap(*(uint32_t *) cursor);
        switch (token) {
        case FDT_BEGIN_NODE:
            if (enterNode(&cursor, callback) != FDT_SUCCESS) {
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
            return FDT_SUCCESS;
        default:
            return FDT_PARSE_ERROR;
        }
    }

    return FDT_SUCCESS;
}

DeviceTree::ReturnCode DeviceTree::enterNode(const char **cursor_ptr,
                                             DeviceTree::TraverseCallback cb)
{
    bool reach_node_end = false;
    *cursor_ptr += 4;  // skip FDT_BEGIN_NODE token

    const char *node_name = (const char *) (*cursor_ptr);  // extract node name
    int node_name_len = cstr::strlen(node_name);
    *cursor_ptr +=
        util::round_up(node_name_len + 1, 4);  // move cursor to next 4-byte aligned address

    while (!reach_node_end) {
        uint32_t token = util::endian_swap(*(uint32_t *) (*cursor_ptr));
        switch (token) {
        case FDT_BEGIN_NODE:
            if (FDT_SUCCESS != enterNode(cursor_ptr, cb)) {
                return FDT_PARSE_ERROR;
            }
            break;
        case FDT_END_NODE:
            reach_node_end = true;
            *cursor_ptr += 4;  // skip FDT_END_NODE token
            break;
        case FDT_PROP:
            if (FDT_SUCCESS != enterProperty(cursor_ptr, node_name, cb)) {
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
    return FDT_SUCCESS;
}

DeviceTree::ReturnCode DeviceTree::enterProperty(const char **cursor_ptr,
                                                 const char *node_name,
                                                 DeviceTree::TraverseCallback cb)
{
    *cursor_ptr += 4;  // skip FDT_PROP token
    FdtProperty *prop = (FdtProperty *) (*cursor_ptr);
    uint32_t prop_len = util::endian_swap(prop->len);
    uint32_t prop_nameoff = util::endian_swap(prop->nameoff);
    const char *prop_name = (const char *) dtbStringBlock + prop_nameoff;
    const char *prop_value = (const char *) (*cursor_ptr + sizeof(FdtProperty));
    cb(node_name, prop_name, prop_value, prop_len);
    *cursor_ptr += sizeof(FdtProperty) + util::round_up(prop_len, 4);

    return FDT_SUCCESS;
}
