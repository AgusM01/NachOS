/// Implementacion de la consola sincronizada.

#include "synch_console.hh"
#include "lib/utility.hh"
#include "threads/semaphore.hh"


/// Constructor de la consola sincronizada. 
SynchConsole::SynchConsole(const char *readFile, const char *writeFile, void *callArg){
    
    cons = new Console(readFile, writeFile, (VoidFunctionPtr)&ReadAvail,
            (VoidFunctionPtr)&WriteDone, callArg);

    WriteMutex = new Lock("SynchConsoleWriteMutex");
    ReadMutex = new Lock("SynchConsoleReadMutex");
    writeDone = new Semaphore("WriteDoneSem", 0);
    readAvail = new Semaphore("ReadAvailSem", 0);

}

/// Desturctor de la consola sincronizada.
SynchConsole::~SynchConsole(){
    
    delete cons;
    delete WriteMutex;
    delete ReadMutex;
    delete writeDone;
    delete readAvail;

}

void
SynchConsole::WriteSync(char ch){

    WriteMutex->Acquire(); 
    cons->PutChar(ch);
    writeDone->P();
    WriteMutex->Release();
    return;

}

char
SynchConsole::ReadSync(){
   
    readAvail->P();
    char ch = cons->GetChar();
    return ch;
}

void
SynchConsole::ReadAvail(void *arg){
    
    readAvail->V();
    return;

}

void
SynchConsole::WriteDone(void *arg)
{
    writeDone->V();
    return;

}

