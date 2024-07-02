#include "util.h"
#include "hint.h"

#define _to_little(u_ptr, ret_ptr, bytes){\
    for(int i = 0; i < bytes; i++){\
        (ret_ptr)[i] = (u_ptr)[(bytes) - i - 1];\
    }\
}

uint32_t to_little_u32(const uint32_t u32){
    uint32_t ret = 0;
    _to_little((uint8_t*)&u32, (uint8_t*)&ret, sizeof(uint32_t));
    return ret;
}

uint64_t to_little_u64(const uint64_t u64){
    uint64_t ret = 0;
    _to_little((uint8_t*)&u64, (uint8_t*)&ret, sizeof(uint64_t))
    return ret;
}
