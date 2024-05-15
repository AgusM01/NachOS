#include "syscall.h"
#include <stdio.h>

int main(void){
        
    char buf[12];
    Read(buf, 12, 0);
    Write(buf, 12, 1);    
    Halt();

}
