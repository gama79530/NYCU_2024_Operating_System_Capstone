#include "types.h"

uint32_t get32(uint64_t addr);
void put32(uint64_t addr, uint32_t val);

void wait_cycles(uint64_t n);   // Wait N CPU cycles (ARM CPU only)
void wait_msec(uint64_t n);
