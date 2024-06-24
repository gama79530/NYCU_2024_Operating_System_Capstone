#include "delay.h"

void wait_cycles(unsigned int n){
    if(n){
        while(n--){
            asm volatile("nop"); 
        }
    }
}