#include "string.h"

/* Private types */

/* Private function declarations */

/* Private data */

/* Function implementations */
int strcmp(const char *lhs, const char *rhs)
{
    while (*lhs != '\0' && *lhs == *rhs) {
        lhs++;
        rhs++;
    }

    return (unsigned char) *lhs - (unsigned char) *rhs;
}
