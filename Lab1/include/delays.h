void wait_cycles(unsigned int n);   // Wait N CPU cycles (ARM CPU only)
void wait_msec(unsigned int n);     // Wait N microsec (ARM CPU only)
unsigned long get_system_timer();
void wait_msec_st(unsigned int n);