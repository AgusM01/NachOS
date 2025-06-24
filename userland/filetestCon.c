#include "syscall.h"
#include "./lib.c"

#define PROCS 20

int
main(void)
{
    int pids[PROCS];
    int res[PROCS];
    char* ito[2];
    char* fst = "1";
    ito[0] = fst;

    Create("FSTest1");
    Create("FSTest2");
    
    for (int i = 0; i < PROCS; i++){
        itoa(i,ito[1]);
        if (i < 10){
            if (!(i % 2)){
                pids[i] = Exec2("wrFile1", ito, 1);
            }
            else
                pids[i] = Exec2("rdFile1", ito, 1);
        }
        else{
            if (!(i % 2))
                pids[i] = Exec2("wrFile2", ito, 1);
            else
                pids[i] = Exec2("rdFile2", ito, 1);
        }
    }

    for (int i = 0; i < PROCS; i++)
        res[i] = Join(pids[i]);

    return 0;
}
