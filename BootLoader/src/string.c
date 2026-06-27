#include "string.h"

int strcmp(const char *lhs, const char *rhs)
{
    while (*lhs != '\0' && *lhs == *rhs) {
        lhs++;
        rhs++;
    }

    return *(const unsigned char *)lhs - *(const unsigned char *)rhs;
}
