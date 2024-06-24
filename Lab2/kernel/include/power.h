#ifndef POWER_H
#define POWER_H

#include "mmio.h"

#define PM_RSTC         ((volatile unsigned int*)(MMIO_BASE+0x0010001c))
#define PM_RSTS         ((volatile unsigned int*)(MMIO_BASE+0x00100020))
#define PM_WDOG         ((volatile unsigned int*)(MMIO_BASE+0x00100024))

#define PM_MAGIC        0x5a000000
#define PM_RSTC_FULLRST 0x00000020

void power_reset(unsigned int ticks);
void power_reset_cancel();

#endif