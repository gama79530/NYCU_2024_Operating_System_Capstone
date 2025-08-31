#ifndef __STRING_H__
#define __STRING_H__
#include "types.h"

size_t strlen(const char *s);
void *memcpy(void *dest, const void *src, size_t n);
char *strcpy(char *dst, const char *src);
char *strncpy(char *s1, const char *s2, size_t n);
int32_t strcmp(const char *s1, const char *s2);
int32_t strncmp(const char *s1, const char *s2, size_t n);

char* uint64_to_hex_str(uint64_t value, uint32_t width, char *buf);
char* uint64_to_dec_str(uint64_t value, uint32_t width, char *buf);

#endif
