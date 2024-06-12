/// Test routines for demonstrating that Nachos can load a user program and
/// execute it.
///
/// Also, routines for testing the Console hardware device.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "threads/semaphore.hh"
#include "threads/system.hh"

#include <stdio.h>


/// Run a user program.
///
/// Open the executable, load it into memory, and jump to it.
void
StartProcess(const char *filename)
{
    ASSERT(filename != nullptr);

    OpenFile *executable = fileSystem->Open(filename);
    if (executable == nullptr) {
        printf("Unable to open file %s\n", filename);
        return;
    }

    AddressSpace *space = new AddressSpace(executable);
    currentThread->space = space;

    currentThread->SetPid(space_table->Add(currentThread));

    #ifndef USE_DL
    delete executable;
    #endif

    space->InitRegisters();  // Set the initial register values.
    space->RestoreState();   // Load page table register.
    
    DEBUG('m', "About to run\n");
    machine->Run();  // Jump to the user progam.
    ASSERT(false);   // `machine->Run` never returns; the address space
                     // exits by doing the system call `Exit`.
}

/// Data structures needed for the console test.
///
/// Threads making I/O requests wait on a `Semaphore` to delay until the I/O
/// completes.

//static SynchConsole *console;
// static Semaphore *readAvail;
// static Semaphore *writeDone;

/// Console interrupt handlers.
///
/// Wake up the thread that requested the I/O.

//static void
//testSyncWrite(){
//    console->WriteSync('a');
//}

/// Test the console by echoing characters typed at the input onto the
/// output.
///
/// Stop when the user types a `q`.
void
ConsoleTest(const char *in, const char *out)
{
    // console   = new SynchConsole(in, out, 0);
    // readAvail = new Semaphore("read avail", 0);
    // writeDone = new Semaphore("write done", 0);

    for (;;) {
        char ch = synch_console->ReadSync();
        synch_console->WriteSync(ch);  // Echo it!
        if (ch == 'q') {
            return;  // If `q`, then quit.
        }
    }
}
