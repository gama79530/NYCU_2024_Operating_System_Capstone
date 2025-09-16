#ifndef LAB2_C_UTIL_H
#define LAB2_C_UTIL_H
#include "types.h"

extern uint64_t get_cpu_id(void);
extern void memzero(void *ptr, uint64_t size);
extern uint32_t get32(uint64_t addr);
extern void put32(uint64_t addr, uint32_t val);

void wait_cycles(uint64_t n);

#define swap(a, b)                \
    do {                          \
        __typeof__(a) temp = (a); \
        (a) = (b);                \
        (b) = temp;               \
    } while (0)


void endian_swap(const void *src, void *dst, uint8_t size);

static inline uint32_t endian_swap32(uint32_t x)
{
    uint32_t y;
    endian_swap(&x, &y, sizeof(x));
    return y;
}

#define is_power_of_two(n) (n > 0 && (n & (n - 1)) == 0)
uint64_t round_up(uint64_t num, uint64_t align);
uint64_t round_down(uint64_t num, uint64_t align);

uint64_t strtol(const char *nptr, char **endptr, int base);
uint64_t strtonum(const char *nptr, int base, int8_t len);

#endif
