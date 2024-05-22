#include "../userprog/syscall.h"

#define ARG_LIMIT 10

unsigned strlen(const char* str){
    unsigned ret;
    for(ret = 0; str[ret] != '\0'; ret++);
    return ret;
}

int puts(const char* str){
    unsigned len = strlen(str);
    return Write(str, len , CONSOLE_OUTPUT);
}

void itoa(int n, char* str){
    // toma un n, por ejemplo 234 y imprime 234
    // 0xffffffff
    int count = 1;
    int sign = n >> 31;
    int div = n > 0 ? n : -1 * n;
    int mod = div % 10;

    //Si el n es 7 count = 1, si el es 234
    for(int i = 10; i < div ; i *= 10 ) {
        count++;
    }   
    
    if (sign){
        str[0] = '-';
        count++;
    }

    while ((div /= 10)) {
        str[--count] = mod + '0';
        mod = div % 10;
    }
    str[--count] = mod + '0';
}