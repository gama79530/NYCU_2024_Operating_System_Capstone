#include "string.h"

int strcmp(const char *s1, const char *s2){
    const unsigned char *us1 = (const unsigned char*) s1;
    const unsigned char *us2 = (const unsigned char*) s2;

    while(*us1 != '\0' && *us1 == *us2){
        us1++;
        us2++;
    }

    return (*us1 > *us2) - (*us1 < *us2);
}

int strncmp(const char *s1, const char *s2, int num){
    const unsigned char *us1 = (const unsigned char*) s1;
    const unsigned char *us2 = (const unsigned char*) s2;

    for(int i = 0; i < num; i++){
        if(us1[i] != us2[i]){
            return (us1[i] > us2[i]) - (us1[i] < us2[i]);
        }else if(us1[i] == '\0'){
            break;
        }
    }

    return 0;
}