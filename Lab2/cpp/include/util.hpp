#ifndef LAB2_CPP_UTIL_HPP
#define LAB2_CPP_UTIL_HPP
#include "types.hpp"

namespace util
{
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

extern "C" {
uint64_t get_cpu_id(void);
void memzero(void *ptr, uint64_t size);
uint32_t get32(uint64_t addr);
void put32(uint64_t addr, uint32_t val);
}

void wait_cycles(uint64_t n);

}  // namespace util

#endif
