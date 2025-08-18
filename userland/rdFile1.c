#include "./lib.c"
#include "syscall.h"
#include <stdio.h>

int main()
{
    
    LsDir(".");
    int fd1 = Open("FSTest1");
    char readed[24];
    if (fd1 != -1)
        Read(readed, 23, fd1);
    CDir("SubDir1");
    int fd2 = Open("FSTest2");
    if (fd2 != -1)
        Read(readed, 23, fd2);
    CDir("..");
    LsDir(".");
    if (fd1 != -1)
        Close(fd1);
    if (fd2 != -1)
        Close(fd2);

    return 0;
}
