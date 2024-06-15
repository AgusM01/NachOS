/// Copyright (c) 2018-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "utility.hh"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


Debug debug;

//Sacado de ChatGPT xd
char* concat(const char* str1, const char* str2) {
    // Calculate the total length needed for the concatenated string
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    size_t totalLen = len1 + len2 + 1; // +1 for the null terminator
    
    // Allocate memory for the concatenated string
    char* result = (char*)malloc(totalLen);
    if (result == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    
    // Copy the first string into the result
    strcpy(result, str1);
    
    // Concatenate the second string onto the end of the first
    strcat(result, str2);
    
    return result;
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
