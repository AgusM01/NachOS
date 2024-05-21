#include "syscall.h"

int
main(void)
{
    SpaceId    newProc;
    OpenFileId input  = CONSOLE_INPUT;
    OpenFileId output = CONSOLE_OUTPUT;
    char       prompt[2] = { '-', '-' };
    char       ch, buffer[60];
    int        i;
    int        status;

    for (;;) {
        Write(prompt, 2, output);
        i = 0;
        status = 0;

        do {
            Read(&buffer[i], 1, input);
        } while (buffer[i++] != '\n');

        buffer[--i] = '\0';

        if (i > 0) {
            if (i > 2 && buffer[0] == '&' && buffer[1] == ' ')
                status = 1;
            if (-1 != (newProc = Exec(buffer + 2*status, status ? 0 : 1)))
                if (!status)
                    Join(newProc);
        }
    }

    return -1;
}
