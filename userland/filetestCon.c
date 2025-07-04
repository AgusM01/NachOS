#include "syscall.h"
#include "./lib.c"

#define PROCS 20

int
main(void)
{
    int pids[PROCS];
    int res[PROCS];

    MkDir("SubDir1");
    MkDir("SubDir2");
    Create("FSTest1");
    CDir("SubDir1");
    Create("FSTest2");
    CDir("..");
    
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
    
    RmDir("SubDir1");
    RmDir("SubDir2");
  for (int i = 0; i < PROCS; i++)
       res[i] = Join(pids[i]);
    
    Remove("FSTest1");
    Remove("FSTest2");
    
    LsDir(".");
    //Open("FSTest1");

    return 0;
}
