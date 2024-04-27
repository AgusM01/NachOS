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