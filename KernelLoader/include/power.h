#ifndef KERNEL_LOADER_POWER_H
#define KERNEL_LOADER_POWER_H
#include "types.h"

#define PM_MAGIC            0x5a000000
#define PM_RSTC_FULLRST     0x00000020

void power_reset(uint32_t ticks);
void power_reset_cancel();

#endif
