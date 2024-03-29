/// Routines for synchronizing threads.
///
/// The implementation for this primitive does not come with base Nachos.
/// It is left to the student.
///
/// When implementing this module, keep in mind that any implementation of a
/// synchronization routine needs some primitive atomic operation.  The
/// semaphore implementation, for example, disables interrupts in order to
/// achieve this; another way could be leveraging an already existing
/// primitive.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "lock.hh"
#include "semaphore.hh"
#include "system.hh"
#include <limits>
#include <stdio.h>

/// Dummy functions -- so we can compile our later assignments.

Lock::Lock(const char *debugName)
{
    mutex = new Semaphore(NULL, 1);
    name = NULL;    
}

Lock::~Lock()
{
    /*el mutex creado por new en constructor.*/
    delete mutex;
}

const char *
Lock::GetName() const
{
    return name;
}

void
Lock::Acquire()
{
    IntStatus oldLevel = interrupt->SetLevel(INT_OFF);
    ASSERT(!(IsHeldByCurrentThread()));
    mutex->P();
    name = currentThread->GetName();
    interrupt->SetLevel(oldLevel);  // Re-enable interrupts.
}

void
Lock::Release()
{
    IntStatus oldLevel = interrupt->SetLevel(INT_OFF);

    ASSERT(IsHeldByCurrentThread());
    mutex->V();
    name = NULL;
    interrupt->SetLevel(oldLevel);  // Re-enable interrupts.

}

bool
Lock::IsHeldByCurrentThread() const
{
    return  (GetName() == currentThread->GetName());
    //return false;
}
