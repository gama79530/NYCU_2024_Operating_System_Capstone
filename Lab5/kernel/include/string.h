#ifndef __STRING_H__
#define __STRING_H__
#include "types.h"

uint64_t str_len(const char *s);

uint64_t hex_str_to_uint(char *s, int hex_str_len);
uint64_t dec_str_to_uint(char *s, int dec_str_len);

char* uint_to_hex_str(uint64_t n, int padding_len, char *dst);
char* uint_to_dec_str(uint64_t i, char *dst);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, uint64_t len);

char char_to_upper(char c);
char char_to_lower(char c);
void to_upper(char *s);
void to_lower(char *s);

void strcpy(const char *src, char *dest);
void strncpy(const char *src, char *dest, uint64_t len);

#endif