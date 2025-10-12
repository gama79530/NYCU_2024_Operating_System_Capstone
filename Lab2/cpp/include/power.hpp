#ifndef LAB2_CPP_POWER_HPP
#define LAB2_CPP_POWER_HPP
#include "types.hpp"

#define PM_MAGIC        0x5a000000
#define PM_RSTC_FULLRST 0x00000020

namespace power
{
void powerReset(uint32_t ticks);
void powerResetCancel();
}  // namespace power

#endif
