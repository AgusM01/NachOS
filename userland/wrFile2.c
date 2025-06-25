#include "./lib.c"
#include "syscall.h"

int main(int argc, char *argv[])
{
    int fd = Open("FSTest2");
    Write("Hola mundo como va\n", 19, fd);

    Close(fd);

    return 0;
}
