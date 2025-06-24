#include "./lib.c"
#include "syscall.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    printf("Proceso lector FSTest1: %s\n", argv[0]);

    int fd = Open("FSTest1");
    char readed[24];
    puts("Leo: ");
    Read(readed, 23, fd);
    Write(argv[0], 1, fd);

    Close(fd);

    return 0;
}
