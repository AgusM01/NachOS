#include "./lib.c"
#include "syscall.h"

int main()
{
    int fd = Open("FSTest1");
    if (fd != -1){
        Write("Hola mundo como va\n", 19, fd);
        Close(fd);
    }
    
    return 0;
}
