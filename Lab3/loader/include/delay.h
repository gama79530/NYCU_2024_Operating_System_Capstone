#ifndef DELAY_H
#define DELAY_H

void wait_cycles(unsigned int n);   // Wait N CPU cycles (ARM CPU only)
void wait_msec(unsigned int n);     // Wait N microsec (ARM CPU only)

#endif