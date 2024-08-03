#include "string.h"

#define swap(type, a, b){\
    type t = (a);\
    (a) = (b);\
    (b) = t;\
}

static char buffer[11];

char* char_to_hex_str(char c){
    buffer[0] = c >> 4 & 0xF;
    buffer[0] += (buffer[0] > 9 ? 0x37 : 0x30);
    buffer[1] = c & 0xF;
    buffer[1] += (buffer[1] > 9 ? 0x37 : 0x30);
    buffer[2] = '\0';

    return buffer;
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

int strcmp(const char *s1, const char *s2){
    unsigned char* us1 = (unsigned char*)s1;
    unsigned char* us2 = (unsigned char*)s2;

    while(*us1 != '\0' && *us1 == *us2){
        us1++;
        us2++;
    }

    return (*us1 > *us2) - (*us1 < *us2);
}

int strncmp(const char *s1, const char *s2, int len){
    unsigned char* us1 = (unsigned char*)s1;
    unsigned char* us2 = (unsigned char*)s2;

    while(len && *us1 != '\0' && *us1 == *us2){
        us1++;
        us2++;
        len--;
    }

    return (*us1 > *us2) - (*us1 < *us2);
}
