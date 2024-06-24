#include "delay.h"

void wait_cycles(unsigned int n){
    if(n){
        while(n--){
            asm volatile("nop"); 
        }
    }
}

void wait_msec(unsigned int n){
    register unsigned long f, t, r;
    // get the current counter frequency
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(f));
    // read the current counter
    asm volatile ("mrs %0, cntpct_el0" : "=r"(t));
    // calculate required count increase
    unsigned long i = ((f / 1000) * n) / 1000;
    // loop while counter increase is less than i
    do{
        asm volatile ("mrs %0, cntpct_el0" : "=r"(r));
    }while(r - t < i);
}