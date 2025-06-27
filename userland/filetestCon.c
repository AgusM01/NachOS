#include "syscall.h"
#include "./lib.c"

#define PROCS 20

int
main(void)
{
    int pids[PROCS];
    int res[PROCS];

    Create("FSTest1");
    Create("FSTest2");
    
    for (int i = 0; i < PROCS; i++){
        if (i < 10){
            if (!(i % 2)){
                pids[i] = Exec("wrFile1", 1);
            }
            else
                pids[i] = Exec("rdFile1", 1);
        }
        else{
            if (!(i % 2))
                pids[i] = Exec("wrFile2", 1);
            else
                pids[i] = Exec("rdFile2", 1);
        }
    }

    for (int i = 0; i < PROCS; i++)
        res[i] = Join(pids[i]);
    
    Remove("FSTest1");
    Remove("FSTest2");

    return 0;
}
