#include "syscall.h"
#include "lib.c"

int 
main(int argc, char* argv[]) 
{
    char str[11] = {0};
    int n = -2345;
    itoa(n,str);
    Write(str,5,CONSOLE_OUTPUT);
    puts("\n");
}