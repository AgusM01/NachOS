#include "syscall.h"
#include "lib.c"
int main(){
    int pids[5];

    for(int i = 0; i < 2; i++)
        pids[i] = Exec("matmult", 1);
    
    for(int i = 0; i < 2; i++){
        Join(pids[i]);
        puts("termine\n");
    }

    puts("chau\n");
    return 0;
}
