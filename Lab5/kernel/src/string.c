#include "string.h"
#include "util.h"

#define BUFFER_SIZE     32

static char buffer[BUFFER_SIZE] = {0};

uint64_t str_len(const char *s){
    uint64_t len = 0;
    while(*s != '\0'){
        len++;
        s++;
    }
    return len;
}

int strcmp(const char *s1, const char *s2){
    unsigned char* us1 = (unsigned char*)s1;
    unsigned char* us2 = (unsigned char*)s2;

    while(*us1 != '\0' && *us1 == *us2){
        us1++;
        us2++;
    }

    return (*us1 > *us2) - (*us1 < *us2);
}

int strncmp(const char *s1, const char *s2, uint64_t len){
    unsigned char* us1 = (unsigned char*)s1;
    unsigned char* us2 = (unsigned char*)s2;

    while(len && *us1 != '\0' && *us1 == *us2){
        us1++;
        us2++;
        len--;
    }

    return len ? (*us1 > *us2) - (*us1 < *us2) : 0;
}

uint64_t hex_str_to_uint(char *s, int hex_str_len){
    uint64_t n = 0;
    hex_str_len = (hex_str_len < 0 || hex_str_len > 16 ? 16 : hex_str_len);
    strncpy(s, buffer, hex_str_len);
    to_upper(buffer);
    for(int i = 0; i < hex_str_len && buffer[i] != '\0'; i++){
        n <<= 4;
        n |= (buffer[i] > '9' ? buffer[i] - 'A' + 10 : buffer[i] - '0');
    }
    return n;
}

void to_upper(char *s){
    while(*s != '\0'){
        *s = char_to_upper(*s);
        s++;
    }
}

void to_lower(char *s){
    while(*s != '\0'){
        *s = char_to_lower(*s);
        s++;
    }
}

char char_to_upper(char c){
    if(c >= 'a' && c <= 'z'){
        c &= ~0x20;
    }

    return c;
}

char char_to_lower(char c){
    if(c >= 'A' && c <= 'Z'){
        c &= ~0x20;
    }

    return c;
}

char* uint_to_dec_str(uint64_t n, char *dst){
    if(dst == NULL) dst = buffer;
    int begin = 0, end = 0;

    do{
        dst[end++] = n % 10 + '0';
        n /= 10;
    }while(n > 0);
    dst[end--] = '\0';

    while(begin < end){
        swap(dst[begin], dst[end]);
        begin++;
        end--;
    }

    return dst;
}

char* uint_to_hex_str(uint64_t n, int padding_len, char *dst){
    if(dst == NULL) dst = buffer;
    int begin = 0, end = 0;

    do{
        dst[end] = n & 0xF;
        dst[end] += (dst[end] > 9 ? 0x37 : 0x30);
        n >>= 4;
        end++;
    }while(n > 0);

    while(end - begin < padding_len){
        dst[end++] = '0';
    }
    dst[end--] = '\0';

    while(begin < end){
        swap(dst[begin], dst[end]);
        begin++;
        end--;
    }

    return dst;
}

uint64_t dec_str_to_uint(char *s, int dec_str_len){
    uint64_t n = 0;
    dec_str_len = (dec_str_len < 0 || dec_str_len > 10 ? 10 : dec_str_len);
    for(int i = 0; i < dec_str_len && s[i] != '\0'; i++)
        n = 10 * n + (s[i] - '0');
    
    return n;
}

void strcpy(const char *src, char *dest){
    while(true){
        *dest = *src;
        if(*src == '\0')    break;
        src++;
        dest++;
    }
}

void strncpy(const char *src, char *dest, uint64_t len){
    while(len != 0){
        *dest = *src;
        len--;
        if(*src == '\0')    break;
        src++;
        dest++;
    }
}
