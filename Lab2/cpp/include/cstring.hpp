#ifndef LAB2_CPP_CSTRING_HPP
#define LAB2_CPP_CSTRING_HPP
#include "types.hpp"

namespace cstr
{
size_t strlen(const char *s);
size_t strlen(char *s);
void *memcpy(void *dest, const void *src, size_t n);
char *strcpy(char *dst, const char *src);
char *strncpy(char *s1, const char *s2, size_t n);
int32_t strcmp(const char *s1, const char *s2);
int32_t strcmp(char *s1, const char *s2);
int32_t strcmp(const char *s1, char *s2);
int32_t strcmp(char *s1, char *s2);
int32_t strncmp(const char *s1, const char *s2, size_t n);
int32_t strncmp(const char *s1, char *s2, size_t n);
int32_t strncmp(char *s1, const char *s2, size_t n);
int32_t strncmp(char *s1, char *s2, size_t n);

char *uint64ToHexStr(uint64_t value, uint32_t width, char *buf);
char *uint64ToDecStr(uint64_t value, uint32_t width, char *buf);
};  // namespace cstr

#endif  // __STRING_HPP__