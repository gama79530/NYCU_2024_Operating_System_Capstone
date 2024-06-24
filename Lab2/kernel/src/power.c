#include "power.h"

void power_reset(unsigned int ticks){
    *PM_RSTC = (PM_MAGIC | PM_RSTC_FULLRST);    // full reset
    *PM_WDOG = (PM_MAGIC | ticks);              // number of watchdog ticks 
}

void power_reset_cancel(){
    *PM_RSTC = (PM_MAGIC | 0);  // cancel reset
    *PM_WDOG = (PM_MAGIC | 0);  // number of watchdog ticks 
}
