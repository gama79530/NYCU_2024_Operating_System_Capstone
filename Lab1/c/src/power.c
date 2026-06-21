#include "power.h"

#include "config.h"
#include "log.h"
#include "peripheral.h"
#include "types.h"
#include "util.h"

/* Private types */

/* Private function declarations */

/* Private data */
#define PM_PASSWORD 0x5A000000
#define PM_RSTC_FULL_RESET 0x00000020

/* Function implementations */
void power_reboot(void)
{
    LOG_VERBOSE("power", "reboot in %u watchdog ticks", CONFIG_REBOOT_TICKS);

    put32(PM_RSTC, PM_PASSWORD | PM_RSTC_FULL_RESET);
    data_memory_barrier();
    put32(PM_WDOG, PM_PASSWORD | CONFIG_REBOOT_TICKS);
    data_sync_barrier();

    while (true) {
        asm volatile("wfe");
    }
}
