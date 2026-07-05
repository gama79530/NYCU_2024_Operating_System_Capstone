#ifndef LAB2_C_STRING_H
#define LAB2_C_STRING_H

#include "types.h"

int strcmp(const char *lhs, const char *rhs);
int strncmp(const char *lhs, const char *rhs, size_t count);
bool strntoul(const char *str, size_t count, unsigned int base, unsigned long *value);

#endif
