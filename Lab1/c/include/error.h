#ifndef LAB1_C_ERROR_H
#define LAB1_C_ERROR_H

#include "types.h"

/* Convert contiguous error codes -1, -2, ... to indexes 0, 1, ... */
static inline size_t error_code_to_index(int error_code)
{
    return (size_t) ~error_code;
}

#endif
