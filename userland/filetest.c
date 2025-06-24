/// Simple program to test whether running a user program works.
///
/// Just do a “syscall” that shuts down the OS.
///
/// NOTE: for some reason, user programs with global data structures
/// sometimes have not worked in the Nachos environment.  So be careful out
/// there!  One option is to allocate data structures as automatics within a
/// procedure, but if you do this, you have to be careful to allocate a big
/// enough stack to hold the automatics!


#include "syscall.h"
#include "./lib.c"

int
main(void)
{
    Create("FSTest1");
    OpenFileId fs1 = Open("FSTest1");
    Create("FSTest2");
    OpenFileId fs2 = Open("FSTest2");
    Write("Hello world\n",12,fs1);
    Write("Agustin\n",8,fs1);
    Write("Merino\n",7,fs2);
    Write("La indómita luz se hizo carne en mí\n",36,fs1);
    Write("La indómita luz se hizo carne en mí\n",36,fs2);
    Write("La indómita luz se hizo carne en mí\n",36,fs1);
    Write("La indómita luz se hizo carne en mí\n",36,fs2);
    Write("La indómita luz se hizo carne en mí\n",36,fs1);
    Write("La indómita luz se hizo carne en mí\n",36,fs2);
    Write("La indómita luz se hizo carne en mí\n",36,fs1);
    Write("La indómita luz se hizo carne en mí\n",36,fs2);
    Close(fs1);
    Close(fs2);
    return 0;
}
