#include "./lib.c"
#include "syscall.h"
#include <stdio.h>

int main()
{

    int fd1 = Open("FSTest1");
    char readed[24];
    Read(readed, 23, fd1);
    int fd2 = Open("FSTest2");
    Read(readed, 23, fd2);
    Close(fd1);
    Close(fd2);

    return 0;
}
