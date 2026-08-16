#ifndef LAB4_C_STRING_H
#define LAB4_C_STRING_H

#include "types.h"

/* Compare two null-terminated strings. */
int strcmp(const char *lhs, const char *rhs);

/* Compare at most count characters from two strings. */
int strncmp(const char *lhs, const char *rhs, size_t count);

/* Return the length of a null-terminated string. */
size_t strlen(const char *str);

/*
 * Bounded unsigned integer parser.
 * Unlike stdlib strtoul(), this reads exactly count characters and returns
 * false if any character is invalid for the base.
 */
bool strntoul(const char *str, size_t count, unsigned int base, unsigned long *value);

#endif
