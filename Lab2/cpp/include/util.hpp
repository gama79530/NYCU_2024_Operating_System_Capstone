#ifndef LAB2_CPP_UTIL_HPP
#define LAB2_CPP_UTIL_HPP
#include "types.hpp"

namespace util
{
extern "C" {
uint64_t get_cpu_id(void);
void memzero(void *ptr, uint64_t size);
uint32_t get32(uint64_t addr);
void put32(uint64_t addr, uint32_t val);
}

template <typename T>
T &&move(T &t)
{
    return static_cast<T &&>(t);
}

template <typename T>
void swap(T &a, T &b)
{
    T temp = move(a);
    a = move(b);
    b = move(temp);
}

template <typename T>
T endian_swap(const T &value)
{
    T ret;
    const char *src = reinterpret_cast<const char *>(&value);
    char *dst = reinterpret_cast<char *>(&ret);
    for (size_t i = 0; i < sizeof(T); i++) {
        dst[i] = src[sizeof(T) - 1 - i];
    }
    return ret;
}

void wait_cycles(uint64_t n);

bool inline is_power_of_two(uint64_t n)
{
    return n > 0 && (n & (n - 1)) == 0;
}

uint64_t round_up(uint64_t num, uint64_t align);
uint64_t round_down(uint64_t num, uint64_t align);

}  // namespace util

#endif
