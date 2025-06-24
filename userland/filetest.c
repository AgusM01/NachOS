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
    OpenFileId o = Open("FSTest1");
    Write("Hello world\n",12,o);
    Write("Agustin\n",8,o);
    Write("Merino\n",7,o);
    Write("La indomita luz se hizo carne en mi\n",36,o);
    Write("La indomita luz se hizo carne en mi\n",36,o);
    Write("La indomita luz se hizo carne en mi\n",36,o);
    Write("La indomita luz se hizo carne en mi\n",36,o);
    Write("La indomita luz se hizo carne en mi\n",36,o);
    Write("La indomita luz se hizo carne en mi\n",36,o);
    Close(o);
    return 0;
}
