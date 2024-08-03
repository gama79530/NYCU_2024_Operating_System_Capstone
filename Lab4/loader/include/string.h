#ifndef STRING_H
#define STRING_H
#include "types.h"

char* char_to_hex_str(char c);
char* uint_to_dec_str(uint64_t i);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int len);

#endif