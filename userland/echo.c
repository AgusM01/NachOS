/// Outputs arguments entered on the command line.

#include "syscall.h"


unsigned
StringLength(const char *s)
{
    // What if `s` is null?

    unsigned i;
    for (i = 0; s[i] != '\0'; i++) {
        Write(s + 1, 1, CONSOLE_OUTPUT);
    }
    return i;
}

int
PrintString(const char *s)
{
    // What if `s` is null?

    unsigned len = StringLength(s);
    return Write(s, len, CONSOLE_OUTPUT);
}

int
PrintChar(char c)
{
    return Write(&c, 1, CONSOLE_OUTPUT);
}

int
main(int argc, char *argv[])
{
    PrintString(argv[0]);
    PrintString(argv[1]);
    for (unsigned i = 0; i < argc; i++) {
        if (i != 0) {
            PrintChar(' ');
        }
        PrintString(argv[i]);
    }
    PrintChar('\n');
}
