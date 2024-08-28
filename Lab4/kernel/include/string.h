#ifndef STRING_H
#define STRING_H
#include "types.h"

char* char_to_hex_str(char c);
char* int_to_hex_str(int i);
char* long_to_hex_str(long l);

uint64_t hex_str_to_uint(char *s);

char* uint_to_dec_str(uint64_t i);

uint64_t dec_str_to_uint(char *s);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, uint32_t len);

int str_len(const char *s);

void strcpy(const char *src, char *dest);
void strncpy(const char *src, char *dest, uint32_t len);

char char_to_upper(char c);
char char_to_lower(char c);

void to_upper(char *s);
void to_lower(char *s);

#endif