/// Implementacion de la consola sincronizada.

#include "synch_console.hh"
#include "lib/utility.hh"
#include "threads/semaphore.hh"
#include <climits>

//Dummys functions :)
static void 
WriteFile (void* s)
{
    Semaphore *writeDone = ((Semaphore**)s)[0];
    writeDone->V();
}

static void 
ReadFile (void* s)
{
    Semaphore *readDone = ((Semaphore**)s)[1];
    readDone->V();
}

/// Constructor de la consola sincronizada. 
SynchConsole::SynchConsole(const char *readFile, const char *writeFile){
    

    WriteMutex = new Lock("SynchConsoleWriteMutex");
    ReadMutex = new Lock("SynchConsoleReadMutex");
    writeDone = new Semaphore(nullptr, 0);
    readAvail = new Semaphore(nullptr, 0);

    cons = new Console(readFile, writeFile, ReadFile, WriteFile, (void*)&writeDone);
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

}

char
SynchConsole::ReadSync(){
   
    readAvail->P();
    char ch = cons->GetChar();
    return ch;
}

