#include "./lib.c"
#include "syscall.h"

int main(int argc, char *argv[])
{
    CDir("SubDir1");
    LsDir(".");
    int fd = Open("FSTest2");
    if (fd != -1){
        Write("Hola mundo como va\n", 19, fd);
        Close(fd);
    }

    return 0;
}
