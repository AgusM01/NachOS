#include "syscall.h"
#include "lib.c"

int 
main(int argc, char* argv[]) 
{
    int readOk;
    char c[1];

    if (argc == 3) {

        OpenFileId original; 
        OpenFileId copia;

        if((original = Open(argv[1])) == -1){
            puts("Error: Open ");
            puts(argv[1]);
            puts("\n");
            Exit(-1);
        }

        if(Create(argv[2]) == -1){
            puts("Error: Create");
            puts(argv[2]);
            puts("\n");
            Exit(-1);
        }

        if((copia = Open(argv[2])) == -1){
            puts("Error: Open ");
            puts(argv[2]);
            puts("\n");
            Exit(-1);
        }

        while((readOk = Read(c, 1, original))){
            Write(c,1,copia);
        }

        if(Close(original) == -1){
            puts("Error: Close\n");
            Exit(-1);
        }
        if(Close(copia) == -1){
            puts("Error: Close\n");
            Exit(-1);
        }
    } else {
        puts("Error: malos argumentos dados\n");
    }
    return 0;
}