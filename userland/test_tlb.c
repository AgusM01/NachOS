#include "syscall.h"
#include "lib.c"

#define PROCS 3

int main(){
    puts("HOLA\n");
    int pids[PROCS];
    int res[PROCS];
    char ito[12] = {0};

    for(int i = 0; i < PROCS; i++){
         //puts("Hola\n");
        pids[i] = Exec("matmult", 1);
        // itoa(pids[i], ito);
        // puts(ito);
        // puts("\n");
    }
    
    for(int i = 0; i < PROCS; i++){
        res[i] = Join(pids[i]);
    }

    // for(int i = 0; i < PROCS; i++){
    //     itoa(res[i], ito);
    //     puts(ito);
    //     puts("\n");
    // }

    return 0;
}
