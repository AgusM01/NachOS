#include "syscall.h"
#include "./lib.c"

int
main(void)
{
    Create("FSTest1");
    LsDir(".");
    MkDir("SubDir1");
    OpenFileId fs1 = Open("FSTest1");
    CDir("SubDir1");
    Create("FSTest2");
    LsDir(".");
    OpenFileId fs2 = Open("FSTest2");
    Write("Hello world\n",12,fs1);
    Write("Agustin\n",8,fs2);
    Write("Merino\n",7,fs2);
    Write("La indómita luz se hizo carne en mí\n",36,fs1);
    Close(fs1);
    Close(fs2);
    Remove("FSTest2");
    LsDir(".");
    CDir("..");
    Remove("FSTest1");
    LsDir(".");
    RmDir("SubDir1");
    return 0;
}
