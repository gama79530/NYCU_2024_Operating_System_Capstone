#include "util.hpp"
#include "common.hpp"

namespace util
{
void wait_cycles(uint64_t n)
{
    for (uint64_t i = 0; i < n; i++) {
        asm volatile("nop");
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

}  // namespace util
