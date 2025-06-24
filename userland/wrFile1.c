#include "./lib.c"
#include "syscall.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    printf("Proceso escritor FSTest1: %s", argv[0]);

    int fd = Open("FSTest1");
    Write("Soy el proceso numero: ", 23, fd);
    Write(argv[0], 1, fd);

    Close(fd);

    return 0;
}
