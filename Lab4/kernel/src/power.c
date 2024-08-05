#include "power.h"
#include "peripheral.h"
#include "util.h"

void power_reset(uint32_t ticks){
    put32(PM_RSTC, PM_MAGIC | PM_RSTC_FULLRST); // full reset
    put32(PM_WDOG, PM_MAGIC | ticks);           // number of watchdog ticks 
}

void power_reset_cancel(){
    put32(PM_RSTC, PM_MAGIC);   // cancel reset
    put32(PM_WDOG, PM_MAGIC);   // number of watchdog ticks 
}
