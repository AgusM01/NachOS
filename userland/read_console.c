#include "syscall.h"
#include <stdio.h>

int main(void){
        
    Write("Hello world\n", 12, 1);
    char buf[13];
    Read(buf, 12, 0);
    printf("%s\n", buf);    
    Halt();

}
