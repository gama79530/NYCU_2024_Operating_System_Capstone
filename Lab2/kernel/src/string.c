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
    num_to_hex_str(c, 1);
    return buffer;
}

char* int_to_hex_str(int n){
    num_to_hex_str(n, 4);
    return buffer;
}

#define hex_str_to_num(c, n, bytes){\
    for(int i = 0; i < (2 * bytes) && (c)[i] != '\0'; i++){\
        (n) <<= 4;\
        (n) |= ((c)[i] > '9' ? (c)[i] - 'A' + 10  : (c)[i] - '0');\
    }\
}

int hex_str_to_uint(char *c){
    unsigned int n = 0;
    hex_str_to_num(c, n, 4);

    return n;
}

char* uint_to_dec_str(unsigned int i){
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

    return len ? (*us1 > *us2) - (*us1 < *us2) : 0;
}
