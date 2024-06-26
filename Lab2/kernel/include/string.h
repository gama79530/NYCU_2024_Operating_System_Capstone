#ifndef STRING_H
#define STRING_H

char* char_to_hex_str(char c);
char* int_to_hex_str(int i);

int hex_str_to_uint(char *c);

char* uint_to_dec_str(unsigned int i);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int len);

#endif