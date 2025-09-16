#include "util.h"
#include "common.h"

void wait_cycles(uint64_t n)
{
    for (uint64_t i = 0; i < n; i++) {
        asm volatile("nop");
    }
}

void endian_swap(const void *src, void *dst, uint8_t size)
{
    const char *s = (const char *) src;
    char *d = (char *) dst;
    for (uint8_t i = 0; i < size; i++) {
        d[i] = s[size - 1 - i];
    }
}

uint64_t round_up(uint64_t num, uint64_t align)
{
    return is_power_of_two(align) ? (num + align - 1) & ~(align - 1)
                                  : (num + align - 1) / align * align;
}

uint64_t round_down(uint64_t num, uint64_t align)
{
    return is_power_of_two(align) ? num & ~(align - 1) : num / align * align;
}

uint64_t strtol(const char *nptr, char **endptr, int base)
{
    uint64_t ret = 0;
    const char *p = nptr;
    int8_t digit;
    while ((digit = digit_to_num(*p)) >= 0 && digit < base) {
        ret = ret * base + digit;
        p++;
    }
    if (endptr != NULL) {
        *endptr = (char *) p;
    }

    return ret;
}

uint64_t strtonum(const char *nptr, int base, int8_t len)
{
    uint64_t ret = 0;
    const char *p = nptr;
    int8_t digit;
    for (int i = 0; i < len && (digit = digit_to_num(*p)) >= 0 && digit < base; i++) {
        ret = ret * base + digit;
        p++;
    }

    return ret;
}
