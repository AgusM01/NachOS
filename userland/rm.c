#include "../userprog/syscall.h"
#include "lib.c"

int 
main(int argc, char* argv[]) {
    int rmOk = 0;
    if (argc < 2) {
        puts("Error: Comando incompleto\n");
    } else {
        for(int countFiles = 1; countFiles < argc; countFiles++){
            rmOk = Remove(argv[countFiles]);
            if (rmOk != 0)
                puts("Error: Remove\n");
        }
    }
    return 0;
}