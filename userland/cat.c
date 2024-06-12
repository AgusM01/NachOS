#include "syscall.h"
#include "lib.c"

int 
main(int argc, char* argv[]) 
{
    const OpenFileId INPUT  = CONSOLE_INPUT;
    const OpenFileId OUTPUT = CONSOLE_OUTPUT;
    int readOk;
    char c[1];

    if (argc == 1) {
        while((readOk = Read(c, 1, INPUT)))
            Write(c,1,OUTPUT);
    } else {
        for (int countFile = 1; countFile < argc; countFile++) {
            Write(argv[countFile], strlen(argv[countFile]), OUTPUT);
            OpenFileId id = Open(argv[countFile]);
            if (id < 0) {
                puts("No se encontro: ");
                puts(argv[countFile]);
                puts("\n");
            } else {
                while((readOk = Read(c, 1, id)))
                    Write(c,1,OUTPUT);
                if (readOk < 0)
                    puts("Error Fari, no funcó el Read.\n");
                puts("\n");
                Close(id);
            }   
        }
    }
    return 0;
}