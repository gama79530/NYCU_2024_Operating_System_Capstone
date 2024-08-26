#include "string.h"

#define swap(type, a, b){\
    type t = (a);\
    (a) = (b);\
    (b) = t;\
}

static char buffer[17] = {0};

#define num_to_hex_str(n, bytes){\
    for(int i = 0; i < 2*(bytes); i++){\
        buffer[i] = ((n) >> (4 * (2 * bytes - i - 1))) & 0xF;\
        buffer[i] += (buffer[i] > 9 ? 0x37 : 0x30);\
    }\
    buffer[2*(bytes)] = '\0';\
}

char* char_to_hex_str(char c){
    num_to_hex_str(c, sizeof(char));
    return buffer;
}

char* int_to_hex_str(int n){
    num_to_hex_str(n, sizeof(int));
    return buffer;
}

char* long_to_hex_str(long l){
    num_to_hex_str(l, sizeof(long));
    return buffer;
}

#define hex_str_to_num(c, n, bytes){\
    for(int i = 0; i < (2 * bytes) && (c)[i] != '\0'; i++){\
        (n) <<= 4;\
        (n) |= ((c)[i] > '9' ? (c)[i] - 'A' + 10  : (c)[i] - '0');\
    }\
}

uint32_t hex_str_to_uint(char *s){
    uint32_t n = 0;
    hex_str_to_num(s, n, 4);

    return n;
}

char* uint_to_dec_str(uint64_t i){
    if(i){
        int begin = 0, end = 0;
        while(i){
            buffer[end++] = i % 10 + '0';
            i /= 10;
        }
        buffer[end--] = '\0';
        while(begin < end){
            swap(char, buffer[begin], buffer[end]);
            begin++;
            end--;
        }
    }else{
        buffer[0] = '0';
        buffer[1] = '\0';
    }

    return buffer;
}


uint64_t dec_str_to_uint(char *s){
    uint64_t num = 0;
    int len = str_len(s);
        
    for(int i = 0; i < len; i++){
        num = 10 * num + (s[i] - '0');
    }

    return num;
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

int strncmp(const char *s1, const char *s2, uint32_t len){
    unsigned char* us1 = (unsigned char*)s1;
    unsigned char* us2 = (unsigned char*)s2;

    while(len && *us1 != '\0' && *us1 == *us2){
        us1++;
        us2++;
        len--;
    }

    return len ? (*us1 > *us2) - (*us1 < *us2) : 0;
}

int str_len(const char *s){
    int len = 0;
    while(*s != '\0'){
        len++;
        s++;
    }
    return len;
}

void strcpy(const char *src, char *dest){
    while(1){
        *dest = *src;
        if(*src == '\0'){
            break;
        }
        src++;
        dest++;
    }
}

void strncpy(const char *src, char *dest, uint32_t len){
    while(1){
        *dest = *src;
        if(!--len || *src == '\0'){
            break;
        }
        src++;
        dest++;
    }
}