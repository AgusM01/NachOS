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
    MkDir("SubDir2");
    Create("FSTest2");
    LsDir(".");
    CDir("SubDir2");
    Create("FSTest3");
    OpenFileId fs3 = Open("FSTest3");
    Write("Hello world fs3\n",16,fs3);
    LsDir(".");
    Close(fs3);
    CDir("..");
    LsDir(".");
    OpenFileId fs2 = Open("FSTest2");
    Write("Hello world\n",12,fs1);
    Write("Agustin\n",8,fs2);
    Write("Merino\n",7,fs2);
    Write("La indómita luz se hizo carne en mí\n",36,fs1);
    Write("La indómita luz se hizo carne en mí\n",36,fs1);
    Close(fs1);
    Close(fs2);
    //Remove("FSTest2");
    LsDir(".");
    CDir("..");
    RmDir("SubDir1");
    Remove("FSTest1");
    LsDir(".");
    return 0;
}
