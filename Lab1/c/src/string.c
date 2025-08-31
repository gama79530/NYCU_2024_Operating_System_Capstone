#include "string.h"
#include "common.h"

static char digit_buffer[21] = {0};

size_t strlen(const char *s)
{
    const char *p = s;
    while (*p != '\0')
        p++;
    return p - s;
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

int32_t strncmp(const char *s1, const char *s2, size_t n)
{
    while (n && *s1 != '\0' && *s1 == *s2) {
        n--;
        s1++;
        s2++;
    }

    return (n ? (*s1 > *s2) - (*s1 < *s2) : 0);
}

char *uint64_to_hex_str(uint64_t value, uint32_t width, char *buf)
{
    char *dst = (buf == NULL ? digit_buffer : buf);
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
        swap(dst[begin], dst[end]);
        begin++;
        end--;
    }

    return dst;
}

char *uint64_to_dec_str(uint64_t value, uint32_t width, char *buf)
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
        swap(dst[begin], dst[end]);
        begin++;
        end--;
    }

    return dst;
}
