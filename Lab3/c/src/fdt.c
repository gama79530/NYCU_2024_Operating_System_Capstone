#include "fdt.h"

#include "config.h"
#include "error.h"
#include "string.h"
#include "util.h"

/* Private constants */
#define FDT_MAGIC 0xD00DFEEDU

/*
 * Flattened Devicetree blob layout:
 *
 *   +-----------------------------------------------------------+
 *   | alignment - 8 bytes,       header                         |
 *   +-----------------------------------------------------------+
 *   |                         free space                        |
 *   +-----------------------------------------------------------+
 *   | alignment - 8 bytes,       memory reservation block       |
 *   +-----------------------------------------------------------+
 *   |                         free space                        |
 *   +-----------------------------------------------------------+
 *   | alignment - 4 bytes,       structure block                |
 *   +-----------------------------------------------------------+
 *   |                         free space                        |
 *   +-----------------------------------------------------------+
 *   | no alignment requirement, strings block                   |
 *   +-----------------------------------------------------------+
 *   |                         free space                        |
 *   +-----------------------------------------------------------+
 *
 * Header layout. All fields are 32-bit big-endian values:
 *
 *   struct fdt_header {
 *       uint32_t magic;
 *       uint32_t totalsize;
 *       uint32_t off_dt_struct;
 *       uint32_t off_dt_strings;
 *       uint32_t off_mem_rsvmap;
 *       uint32_t version;
 *       uint32_t last_comp_version;
 *       uint32_t boot_cpuid_phys;
 *       uint32_t size_dt_strings;
 *       uint32_t size_dt_struct;
 *   };
 */
#define FDT_HEADER_SIZE 40
#define FDT_MAGIC_OFFSET 0
#define FDT_TOTALSIZE_OFFSET 4
#define FDT_OFF_DT_STRUCT_OFFSET 8
#define FDT_OFF_DT_STRINGS_OFFSET 12
#define FDT_SIZE_DT_STRINGS_OFFSET 32
#define FDT_SIZE_DT_STRUCT_OFFSET 36

#define FDT_TOKEN_SIZE 4
#define FDT_PROP_HEADER_SIZE 8
#define FDT_ALIGNMENT 4

/*
 * Structure block layout. Every token and token payload is 4-byte aligned:
 *
 *   node:
 *     FDT_BEGIN_NODE
 *     null-terminated node name
 *     properties
 *     child nodes
 *     FDT_END_NODE
 *
 *   property:
 *     FDT_PROP
 *     uint32_t len      (big-endian)
 *     uint32_t nameoff  (big-endian, offset into strings block)
 *     value bytes
 *
 *   FDT_NOP may appear between entries, and FDT_END closes the block.
 */
#define FDT_BEGIN_NODE 0x00000001U
#define FDT_END_NODE 0x00000002U
#define FDT_PROP 0x00000003U
#define FDT_NOP 0x00000004U
#define FDT_END 0x00000009U

#define FDT_TRAVERSE_STOP ((fdt_error_t) 1)

/* Private types */
typedef struct {
    const char *path;
    const char *name;
    const uint8_t *value;
    size_t size;
    bool found;
    bool path_matches[CONFIG_FDT_MAX_DEPTH];
} fdt_property_lookup_t;

typedef enum {
    FDT_TRAVERSE_BEGIN_NODE,
    FDT_TRAVERSE_END_NODE,
    FDT_TRAVERSE_PROPERTY,
} fdt_traverse_item_type_t;

typedef struct {
    fdt_traverse_item_type_t type;
    int depth;
    const char *node_name;
    const char *property_name;
    const uint8_t *property_value;
    size_t property_size;
} fdt_traverse_item_t;

typedef fdt_error_t (*fdt_callback_t)(const fdt_traverse_item_t *item, void *context);

/* Private function declarations */
/* Read one big-endian 32-bit word from the structure block and advance cursor. */
static bool fdt_read_u32(const uint8_t **cursor, const uint8_t *limit, uint32_t *value);

/* Skip a null-terminated node name and align cursor to the next FDT token. */
static bool fdt_skip_node_name(const uint8_t **cursor, const uint8_t *limit);

/* Skip size bytes and align cursor to the next FDT token boundary. */
static bool fdt_skip_bytes_aligned(const uint8_t **cursor, const uint8_t *limit, size_t size);

/*
 * Walk the FDT structure block and call callback for each node/property item.
 *
 * The traversal keeps only a depth stack; callbacks decide whether to collect
 * data, ignore items, or stop early with FDT_TRAVERSE_STOP.
 */
static fdt_error_t fdt_traverse(const fdt_t *fdt, fdt_callback_t callback, void *context);

/* Parse one FDT_PROP token payload and pass it to the traversal callback. */
static fdt_error_t fdt_parse_property(const fdt_t *fdt,
                                      const uint8_t **cursor,
                                      const uint8_t *limit,
                                      int depth,
                                      const char *node_name,
                                      fdt_callback_t callback,
                                      void *callback_context);

/*
 * Traversal callback used by fdt_get_property().
 *
 * It tracks which depth levels match the requested path and stops traversal
 * once the target property is found.
 */
static fdt_error_t fdt_property_lookup_callback(const fdt_traverse_item_t *item, void *context);

/* Return true when node_name matches the requested path component at depth. */
static bool fdt_path_component_matches(const char *path, int depth, const char *node_name);

/* Return true when the requested path ends exactly at depth. */
static bool fdt_path_ends_at_depth(const char *path, int depth);

/* Return a validated string-block pointer for a property name offset. */
static const char *fdt_string_at(const fdt_t *fdt, uint32_t offset);

/* Private data */
static const char *const fdt_error_messages[] = {
    "Invalid argument.",      "Invalid FDT magic.",      "FDT data is out of range.",
    "Invalid FDT structure.", "FDT property not found.",
};

#define FDT_ERROR_COUNT (sizeof(fdt_error_messages) / sizeof(fdt_error_messages[0]))

/* Function implementations */
const char *fdt_error_string(fdt_error_t error)
{
    if (error >= FDT_SUCCESS) {
        return "Unknown FDT error.";
    }

    size_t index = error_code_to_index(error);

    if (index >= FDT_ERROR_COUNT) {
        return "Unknown FDT error.";
    }

    return fdt_error_messages[index];
}

fdt_error_t fdt_init(fdt_t *fdt, uintptr_t address)
{
    const uint8_t *base;
    uint32_t magic;
    uint32_t total_size;
    uint32_t struct_offset;
    uint32_t strings_offset;
    uint32_t struct_size;
    uint32_t strings_size;

    if (fdt == NULL || address == 0) {
        return FDT_ERROR_INVALID_ARGUMENT;
    }

    base = (const uint8_t *) address;
    magic = read_be32(base + FDT_MAGIC_OFFSET);
    if (magic != FDT_MAGIC) {
        return FDT_ERROR_INVALID_MAGIC;
    }

    total_size = read_be32(base + FDT_TOTALSIZE_OFFSET);
    struct_offset = read_be32(base + FDT_OFF_DT_STRUCT_OFFSET);
    strings_offset = read_be32(base + FDT_OFF_DT_STRINGS_OFFSET);
    struct_size = read_be32(base + FDT_SIZE_DT_STRUCT_OFFSET);
    strings_size = read_be32(base + FDT_SIZE_DT_STRINGS_OFFSET);

    if (total_size < FDT_HEADER_SIZE || struct_offset > total_size || strings_offset > total_size ||
        struct_size > total_size - struct_offset || strings_size > total_size - strings_offset) {
        return FDT_ERROR_OUT_OF_RANGE;
    }

    fdt->base = base;
    fdt->total_size = total_size;
    fdt->struct_begin = base + struct_offset;
    fdt->struct_end = fdt->struct_begin + struct_size;
    fdt->strings_begin = (const char *) base + strings_offset;
    fdt->strings_end = fdt->strings_begin + strings_size;

    return FDT_SUCCESS;
}

static fdt_error_t fdt_traverse(const fdt_t *fdt, fdt_callback_t callback, void *context)
{
    const uint8_t *cursor;
    const char *node_stack[CONFIG_FDT_MAX_DEPTH];
    int depth = -1;

    if (fdt == NULL || callback == NULL) {
        return FDT_ERROR_INVALID_ARGUMENT;
    }

    if (fdt->struct_begin == NULL || fdt->struct_end == NULL) {
        return FDT_ERROR_INVALID_ARGUMENT;
    }

    cursor = fdt->struct_begin;
    while (cursor < fdt->struct_end) {
        uint32_t token;

        if (!fdt_read_u32(&cursor, fdt->struct_end, &token)) {
            return FDT_ERROR_INVALID_STRUCTURE;
        }

        switch (token) {
        case FDT_BEGIN_NODE: {
            fdt_traverse_item_t item;
            fdt_error_t error;
            const char *node_name;

            node_name = (const char *) cursor;
            depth++;
            if (depth >= CONFIG_FDT_MAX_DEPTH) {
                return FDT_ERROR_INVALID_STRUCTURE;
            }

            node_stack[depth] = node_name;
            if (!fdt_skip_node_name(&cursor, fdt->struct_end)) {
                return FDT_ERROR_INVALID_STRUCTURE;
            }

            item.type = FDT_TRAVERSE_BEGIN_NODE;
            item.depth = depth;
            item.node_name = node_name;
            item.property_name = NULL;
            item.property_value = NULL;
            item.property_size = 0;
            error = callback(&item, context);
            if (error != FDT_SUCCESS) {
                return error;
            }
            break;
        }
        case FDT_END_NODE: {
            fdt_traverse_item_t item;
            fdt_error_t error;

            if (depth < 0) {
                return FDT_ERROR_INVALID_STRUCTURE;
            }

            item.type = FDT_TRAVERSE_END_NODE;
            item.depth = depth;
            item.node_name = node_stack[depth];
            item.property_name = NULL;
            item.property_value = NULL;
            item.property_size = 0;
            error = callback(&item, context);
            if (error != FDT_SUCCESS) {
                return error;
            }

            depth--;
            break;
        }
        case FDT_PROP: {
            if (depth < 0) {
                return FDT_ERROR_INVALID_STRUCTURE;
            }

            fdt_error_t error = fdt_parse_property(fdt, &cursor, fdt->struct_end, depth,
                                                   node_stack[depth], callback, context);
            if (error != FDT_SUCCESS) {
                return error;
            }
            break;
        }
        case FDT_NOP:
            break;
        case FDT_END:
            return FDT_SUCCESS;
        default:
            return FDT_ERROR_INVALID_STRUCTURE;
        }
    }

    return FDT_ERROR_INVALID_STRUCTURE;
}

fdt_error_t fdt_get_property(const fdt_t *fdt,
                             const char *path,
                             const char *name,
                             const uint8_t **value,
                             size_t *size)
{
    fdt_error_t error;
    fdt_property_lookup_t lookup;

    if (fdt == NULL || path == NULL || name == NULL || value == NULL || size == NULL ||
        path[0] != '/') {
        return FDT_ERROR_INVALID_ARGUMENT;
    }

    lookup.path = path;
    lookup.name = name;
    lookup.value = NULL;
    lookup.size = 0;
    lookup.found = false;
    for (size_t i = 0; i < CONFIG_FDT_MAX_DEPTH; i++) {
        lookup.path_matches[i] = false;
    }

    error = fdt_traverse(fdt, fdt_property_lookup_callback, &lookup);
    if (error != FDT_SUCCESS && error != FDT_TRAVERSE_STOP) {
        return error;
    }

    if (!lookup.found) {
        return FDT_ERROR_NOT_FOUND;
    }

    *value = lookup.value;
    *size = lookup.size;
    return FDT_SUCCESS;
}

static bool fdt_read_u32(const uint8_t **cursor, const uint8_t *limit, uint32_t *value)
{
    if (*cursor > limit || (size_t) (limit - *cursor) < sizeof(uint32_t)) {
        return false;
    }

    *value = read_be32(*cursor);
    *cursor += sizeof(uint32_t);
    return true;
}

static bool fdt_skip_node_name(const uint8_t **cursor, const uint8_t *limit)
{
    const char *name = (const char *) *cursor;
    size_t length;
    uintptr_t next;

    while (*cursor < limit && **cursor != '\0') {
        (*cursor)++;
    }

    if (*cursor >= limit) {
        return false;
    }

    length = strlen(name) + 1;
    next = align_up((uintptr_t) name + length, FDT_ALIGNMENT);
    if (next > (uintptr_t) limit) {
        return false;
    }

    *cursor = (const uint8_t *) next;
    return true;
}

static bool fdt_skip_bytes_aligned(const uint8_t **cursor, const uint8_t *limit, size_t size)
{
    uintptr_t next;

    if (*cursor > limit || size > (size_t) (limit - *cursor)) {
        return false;
    }

    next = align_up((uintptr_t) *cursor + size, FDT_ALIGNMENT);
    if (next > (uintptr_t) limit) {
        return false;
    }

    *cursor = (const uint8_t *) next;
    return true;
}

static fdt_error_t fdt_parse_property(const fdt_t *fdt,
                                      const uint8_t **cursor,
                                      const uint8_t *limit,
                                      int depth,
                                      const char *node_name,
                                      fdt_callback_t callback,
                                      void *callback_context)
{
    uint32_t length;
    uint32_t name_offset;
    const uint8_t *value;
    const char *name;
    fdt_traverse_item_t item;

    if (!fdt_read_u32(cursor, limit, &length) || !fdt_read_u32(cursor, limit, &name_offset)) {
        return FDT_ERROR_INVALID_STRUCTURE;
    }

    value = *cursor;
    if (!fdt_skip_bytes_aligned(cursor, limit, length)) {
        return FDT_ERROR_INVALID_STRUCTURE;
    }

    name = fdt_string_at(fdt, name_offset);
    if (name == NULL) {
        return FDT_ERROR_OUT_OF_RANGE;
    }

    item.type = FDT_TRAVERSE_PROPERTY;
    item.depth = depth;
    item.node_name = node_name;
    item.property_name = name;
    item.property_value = value;
    item.property_size = length;

    return callback(&item, callback_context);
}

static fdt_error_t fdt_property_lookup_callback(const fdt_traverse_item_t *item, void *context)
{
    fdt_property_lookup_t *lookup = context;

    if (item->type == FDT_TRAVERSE_BEGIN_NODE) {
        if (item->depth == 0) {
            lookup->path_matches[0] = fdt_path_component_matches(lookup->path, 0, item->node_name);
        } else if (lookup->path_matches[item->depth - 1]) {
            lookup->path_matches[item->depth] =
                fdt_path_component_matches(lookup->path, item->depth, item->node_name);
        } else {
            lookup->path_matches[item->depth] = false;
        }
        return FDT_SUCCESS;
    }

    if (item->type == FDT_TRAVERSE_END_NODE) {
        lookup->path_matches[item->depth] = false;
        return FDT_SUCCESS;
    }

    if (item->type != FDT_TRAVERSE_PROPERTY || !lookup->path_matches[item->depth] ||
        !fdt_path_ends_at_depth(lookup->path, item->depth) ||
        strcmp(item->property_name, lookup->name) != 0) {
        return FDT_SUCCESS;
    }

    lookup->value = item->property_value;
    lookup->size = item->property_size;
    lookup->found = true;
    return FDT_TRAVERSE_STOP;
}

static bool fdt_path_component_matches(const char *path, int depth, const char *node_name)
{
    const char *component;
    size_t component_length = 0;

    if (path == NULL || path[0] != '/' || node_name == NULL) {
        return false;
    }

    if (depth == 0) {
        return node_name[0] == '\0';
    }

    component = path + 1;
    for (int current_depth = 1; current_depth < depth; current_depth++) {
        while (*component != '\0' && *component != '/') {
            component++;
        }

        if (*component != '/') {
            return false;
        }

        component++;
    }

    if (*component == '\0' || *component == '/') {
        return false;
    }

    while (component[component_length] != '\0' && component[component_length] != '/') {
        component_length++;
    }

    return strlen(node_name) == component_length &&
           strncmp(node_name, component, component_length) == 0;
}

static bool fdt_path_ends_at_depth(const char *path, int depth)
{
    const char *component;

    if (path == NULL || path[0] != '/') {
        return false;
    }

    if (depth == 0) {
        return path[1] == '\0';
    }

    component = path + 1;
    for (int current_depth = 1; current_depth < depth; current_depth++) {
        while (*component != '\0' && *component != '/') {
            component++;
        }

        if (*component != '/') {
            return false;
        }

        component++;
    }

    while (*component != '\0' && *component != '/') {
        component++;
    }

    return *component == '\0';
}

static const char *fdt_string_at(const fdt_t *fdt, uint32_t offset)
{
    const char *string;

    if ((size_t) offset >= (size_t) (fdt->strings_end - fdt->strings_begin)) {
        return NULL;
    }

    string = fdt->strings_begin + offset;
    while (string < fdt->strings_end) {
        if (*string == '\0') {
            return fdt->strings_begin + offset;
        }
        string++;
    }

    return NULL;
}
