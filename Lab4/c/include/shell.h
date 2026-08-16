#ifndef LAB4_C_SHELL_H
#define LAB4_C_SHELL_H

#include "fdt.h"

/* Run the interactive shell using optional boot devicetree context. */
void shell_run(const fdt_t *fdt);

#endif
