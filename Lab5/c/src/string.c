#include "string.h"

/* Private types */

/* Private function declarations */

/* Convert one ASCII hex digit into its numeric value. */
static bool char_to_digit(char c, unsigned int *digit);

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

int strncmp(const char *lhs, const char *rhs, size_t count)
{
    while (count > 0 && *lhs != '\0' && *lhs == *rhs) {
        lhs++;
        rhs++;
        count--;
    }

    if (count == 0) {
        return 0;
    }

    return (unsigned char) *lhs - (unsigned char) *rhs;
}

size_t strlen(const char *str)
{
    size_t length = 0;

    while (str[length] != '\0') {
        length++;
    }

    return length;
}

bool strntoul(const char *str, size_t count, unsigned int base, unsigned long *value)
{
    unsigned long result = 0;

    if (str == NULL || value == NULL || base < 2 || base > 16) {
        return false;
    }

    for (size_t i = 0; i < count; i++) {
        unsigned int digit;

        if (!char_to_digit(str[i], &digit) || digit >= base) {
            return false;
        }

        result = result * base + digit;
    }

    *value = result;
    return true;
}

static bool char_to_digit(char c, unsigned int *digit)
{
    if (c >= '0' && c <= '9') {
        *digit = (unsigned int) (c - '0');
        return true;
    }

    if (c >= 'a' && c <= 'f') {
        *digit = (unsigned int) (c - 'a' + 10);
        return true;
    }

    if (c >= 'A' && c <= 'F') {
        *digit = (unsigned int) (c - 'A' + 10);
        return true;
    }

    return false;
}
