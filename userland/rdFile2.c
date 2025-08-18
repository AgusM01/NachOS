#include "./lib.c"
#include "syscall.h"
#include <stdio.h>

int main()
{

    CDir("SubDir1");
    LsDir(".");
    int fd2 = Open("FSTest2");
    char readed[24];
    if (fd2 != -1)
        Read(readed, 23, fd2);
    CDir("..");
    LsDir(".");
    int fd1 = Open("FSTest1");
    if (fd1 != -1)
        Write(readed, 23, fd1);
    if (fd1 != -1)
        Close(fd1);
    if (fd2 != -1)
        Close(fd2);

    return 0;
}
