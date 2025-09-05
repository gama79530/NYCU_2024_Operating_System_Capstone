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
}  // namespace util
