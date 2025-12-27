#ifndef LAB3_C_DEVICE_TREE_H
#define LAB3_C_DEVICE_TREE_H
#include "types.h"

/*  FLATTENED DEVICETREE FORMAT

+-----------------------------------------------------------+
|   alignment - 8 bytes,        fdt_header_t                |
+-----------------------------------------------------------+
|                       (free space)                        |
+-----------------------------------------------------------+
|   alignment - 8 bytes,        memory reservation block    |
+-----------------------------------------------------------+
|                       (free space)                        |
+-----------------------------------------------------------+
|   alignment - 4 bytes,        structure block             |
+-----------------------------------------------------------+
|                       (free space)                        |
+-----------------------------------------------------------+
|   no alignment requirement,   strings block               |
+-----------------------------------------------------------+
|                       (free space)                        |
+-----------------------------------------------------------+

References: https://github.com/devicetree-org/devicetree-specification/releases/tag/v0.4
            Chapter 5, 2, 3

            https://github.com/raspberrypi/linux/blob/rpi-5.10.y/arch/arm/boot/dts/bcm2710-rpi-3-b.dts
*/

// All the header fields are 32-bit integers, stored in big-endian format.
typedef struct fdt_header {
    const uint32_t magic;           // 0xD00DFEED
    const uint32_t totalsize;       // size in bytes of the devicetree data structure
    const uint32_t off_dt_struct;   // offset of the structure block
    const uint32_t off_dt_strings;  // offset of the string block
    const uint32_t off_mem_rsvmap;  // offset of the memory reservation block
    const uint32_t version;
    const uint32_t last_comp_version;
    const uint32_t boot_cpuid_phys;
    const uint32_t size_dt_strings;  // length of the structure block
    const uint32_t size_dt_struct;   // length of the strings block
} fdt_header_t;

#define FDT_HEADER_MAGIC 0xD00DFEED

/* memory reservation block:
    1. consists of a list of pairs of 64-bit big-endian integers
    2. terminated with an entry where both address and size are equal to 0.
    3. physical address and size in bytes of a reserved memory region
*/
typedef struct fdt_reserve_entry {
    const uint64_t address;
    const uint64_t size;
} fdt_reserve_entry_t;

/* structure block:

1. All tokens shall be aligned on 4 bytes after the token’s data.
2. The next token of FDT_BEGIN_NODE may be any token except FDT_END.
3. The next token of FDT_END_NODE may be any token except FDT_PROP.
4. FDT_END token marks the end of the structure block.

tree node:

+-------------------------------------------+
|   (optional) FDT_NOP, any number          |
+-------------------------------------------+
|   FDT_BEGIN_NODE                          |
+-------------------------------------------+
|   node name, null-terminated name string  |
+-------------------------------------------+
|   properties of the node                  |
+-------------------------------------------+
|   child nodes                             |
+-------------------------------------------+
|   (optional) FDT_NOP, any number          |
+-------------------------------------------+
|   FDT_END_NODE                            |
+-------------------------------------------+
*/

#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE 0x00000002
#define FDT_PROP 0x00000003
#define FDT_NOP 0x00000004
#define FDT_END 0x00000009

/*  property of node:

+-------------------------------------------+
|   (optional) FDT_NOP, any number          |
+-------------------------------------------+
|   FDT_PROP                                |
+-------------------------------------------+
|   fdt_property_t                          |
+-------------------------------------------+
|   property’s value                        |
+-------------------------------------------+

*/

// both two fields are big-endian uint32_t
typedef struct fdt_property {
    const uint32_t len;      // length of the property’s value in bytes
    const uint32_t nameoff;  // offset into the strings block for property’s name
} fdt_property_t;

int set_dtb_address(uint64_t addr);
fdt_header_t *get_dtb_info(void);

typedef void (*fdt_callback_t)(const char *node_name,
                               const char *property_name,
                               const char *property_value,
                               uint32_t property_len);

#define FDT_HEADER_MAGIC_ERROR -1
#define FDT_HEADER_POINTER_ERROR -2
#define FDT_PARSE_ERROR -3
extern const char *fdt_err_msg[];

int fdt_traverse(fdt_callback_t cb);

#endif
