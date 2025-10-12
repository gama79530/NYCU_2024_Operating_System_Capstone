#include "power.hpp"
#include "common.hpp"
#include "peripheral.hpp"

namespace power
{
void powerReset(uint32_t ticks)
{
    util::put32(PM_RSTC, PM_MAGIC | PM_RSTC_FULLRST);  // full reset
    util::put32(PM_WDOG, PM_MAGIC | ticks);            // number of watchdog ticks
}

void powerResetCancel()
{
    util::put32(PM_RSTC, PM_MAGIC);  // cancel reset
    util::put32(PM_WDOG, PM_MAGIC);  // number of watchdog ticks
}
}  // namespace power
