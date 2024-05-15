/// Implementacion de la consola sincronizada.

#include "synch_console.hh"
#include "lib/utility.hh"
#include "threads/semaphore.hh"
#include <climits>
#include <stdio.h>

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
    writeDone = new Semaphore("WriteDoneSem", 0);
    readAvail = new Semaphore("ReadDoneSem", 0);

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
SynchConsole::WriteNBytes(char* buf, int len){
    
    WriteMutex->Acquire();
    for (int i = 0; i < len; i++){
        cons->PutChar(buf[i]);
        writeDone->P();
    }
    WriteMutex->Release();

}

void
SynchConsole::ReadNBytes(char* buf, int len){
   for (int i = 0; i < len; i++){
    readAvail->P();
    buf[i] = cons->GetChar();
    //ASSERT(buf[i] != EOF);
   }
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

