#include "syscall.h"

#define ARG_LIMIT 10

unsigned strlen(const char* str){
    unsigned ret;
    for(ret = 0; *(str + ret) != '\0'; ret++);
    return ret;
}

void puts(const char* str){
    const OpenFileId OUTPUT = CONSOLE_OUTPUT;
    unsigned len = strlen(str);
    Write(str, len , OUTPUT);
}

void itoa(int n, char* str){
    int count = strlen(str);
    int sign = n >> 31;
    int floor = n > 0 ? n : -1 * n;
    int mod = n % 10;

    if (sign)
        str[0] = '-';

    while ((floor /= 10)) {
        str[--count] = mod + '0';
        mod = floor % 10;
    }
    str[--count] = mod + '0';
}