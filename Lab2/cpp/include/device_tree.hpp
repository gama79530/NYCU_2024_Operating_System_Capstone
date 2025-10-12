#ifndef LAB2_CPP_DEVICE_TREE_HPP
#define LAB2_CPP_DEVICE_TREE_HPP
#include "types.hpp"

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

/* memory reservation block:
    1. consists of a list of pairs of 64-bit big-endian integers
    2. terminated with an entry where both address and size are equal to 0.
    3. physical address and size in bytes of a reserved memory region
*/

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

class DeviceTree
{
public:
    // Callback type definition - more tightly coupled with the class
    using TraverseCallback = void (*)(const char *node_name,
                                      const char *property_name,
                                      const char *property_value,
                                      uint32_t property_len);

    // All the header fields are 32-bit integers, stored in big-endian format.
    struct FdtHeader {
        const uint32_t magic;         // 0xD00DFEED
        const uint32_t totalsize;     // size in bytes of the devicetree data structure
        const uint32_t offDtStruct;   // offset of the structure block
        const uint32_t offDtStrings;  // offset of the string block
        const uint32_t offMemRsvmap;  // offset of the memory reservation block
        const uint32_t version;
        const uint32_t lastCompVersion;
        const uint32_t bootCpuidPhys;
        const uint32_t sizeDtStrings;  // length of the structure block
        const uint32_t sizeDtStruct;   // length of the strings block
    };

    enum ReturnCode {
        FDT_SUCCESS = 0,
        FDT_HEADER_MAGIC_ERROR = -1,
        FDT_HEADER_POINTER_ERROR = -2,
        FDT_PARSE_ERROR = -3,
    };

    static constexpr uint32_t FDT_HEADER_MAGIC = 0xD00DFEED;
    static constexpr uint32_t FDT_BEGIN_NODE = 0x00000001;
    static constexpr uint32_t FDT_END_NODE = 0x00000002;
    static constexpr uint32_t FDT_PROP = 0x00000003;
    static constexpr uint32_t FDT_NOP = 0x00000004;
    static constexpr uint32_t FDT_END = 0x00000009;

    int init(uint64_t addr);
    FdtHeader *getHeader() const;
    ReturnCode traverse(TraverseCallback callback);

    inline const char *errorCodeToStr(ReturnCode errCode)
    {
        return errCode < 0 ? errMessages[~errCode] : nullptr;
    }

private:
    struct FdtReserveEntry {
        const uint64_t address;
        const uint64_t size;
    };

    struct FdtProperty {
        const uint32_t len;  // length of the property value
        const uint32_t
            nameoff;  // offset into the strings block where the name of the property is located
    };

    static const char *const errMessages[];

    bool initialized = false;
    const char *dtbHeader = nullptr;
    const char *dtbMemRsvBlock = nullptr;
    const char *dtbStructBlock = nullptr;
    const char *dtbStringBlock = nullptr;
    uint32_t dtbStructBlockSize = 0;

    FdtHeader *header = nullptr;

    ReturnCode enterNode(const char **cursor_ptr, TraverseCallback cb);
    ReturnCode enterProperty(const char **cursor_ptr, const char *node_name, TraverseCallback cb);
};

#endif
