#include "cstring.hpp"
#include "common.hpp"

extern "C" {
void *memset(void *s, int c, size_t n)
{
    unsigned char uc = (unsigned char)c;
    unsigned char *ptr = (unsigned char *)s;
    
    for (size_t i = 0; i < n; i++) {
        ptr[i] = uc;
    }
    
    return s;
}
}

namespace cstr
{

static char digit_buffer[21] = {0};

size_t strlen(const char *s)
{
    const char *p = s;
    while (*p != '\0')
        p++;
    return p - s;
}

size_t strlen(char *s)
{
    return strlen(const_cast<const char *>(s));
}

void *memcpy(void *dest, const void *src, size_t n)
{
    char *d = (char *) dest;
    const char *s = (const char *) src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

char *strcpy(char *dst, const char *src)
{
    char *d = dst;
    while ((*d++ = *src++))
        ;
    return dst;
}

char *strncpy(char *s1, const char *s2, size_t n)
{
    char *p = s1;
    while (n--) {
        *p++ = (*s2 == '\0' ? '\0' : *s2++);
    }

    return s1;
}

int32_t strcmp(const char *s1, const char *s2)
{
    while (*s1 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (*s1 > *s2) - (*s1 < *s2);
}

int32_t strcmp(char *s1, const char *s2)
{
    return strcmp(const_cast<const char *>(s1), s2);
}

int32_t strcmp(const char *s1, char *s2)
{
    return strcmp(s1, const_cast<const char *>(s2));
}

int32_t strcmp(char *s1, char *s2)
{
    return strcmp(const_cast<const char *>(s1), const_cast<const char *>(s2));
}

int32_t strncmp(const char *s1, const char *s2, size_t n)
{
    while (n && *s1 != '\0' && *s1 == *s2) {
        n--;
        s1++;
        s2++;
    }

    return (n ? (*s1 > *s2) - (*s1 < *s2) : 0);
}

int32_t strncmp(const char *s1, char *s2, size_t n)
{
    return strncmp(s1, const_cast<const char *>(s2), n);
}

int32_t strncmp(char *s1, const char *s2, size_t n)
{
    return strncmp(const_cast<const char *>(s1), s2, n);
}

int32_t strncmp(char *s1, char *s2, size_t n)
{
    return strncmp(const_cast<const char *>(s1), const_cast<const char *>(s2), n);
}

char *uint64ToHexStr(uint64_t value, uint32_t width, char *buf)
{
    char *dst = (buf == nullptr ? digit_buffer : buf);
    if (width > 16) {
        width = 16;
    }
    size_t begin = 0, end = 0;

    do {
        dst[end++] = "0123456789ABCDEF"[value & 0xf];
        value >>= 4;
    } while (value > 0);

    while (end - begin < width) {
        dst[end++] = '0';
    }
    dst[end--] = '\0';

    while (begin < end) {
        util::swap(dst[begin], dst[end]);
        begin++;
        end--;
    }

    return dst;
}

char *uint64ToDecStr(uint64_t value, uint32_t width, char *buf)
{
    char *dst = (buf == NULL ? digit_buffer : buf);
    if (width > 20) {
        width = 20;
    }
    size_t begin = 0, end = 0;
    do {
        dst[end++] = "0123456789"[value % 10];
        value /= 10;
    } while (value > 0);

    while (end - begin < width) {
        dst[end++] = '0';
    }
    dst[end--] = '\0';

    while (begin < end) {
        util::swap(dst[begin], dst[end]);
        begin++;
        end--;
    }

    return dst;
}
}  // namespace cstr