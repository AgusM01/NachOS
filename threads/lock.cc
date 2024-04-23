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
    name = debugName;
    Current = NULL;
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
    DEBUG('t', "Hago Acquire en el Lock %s, soy %s\n",
        this->GetName(),
        currentThread->GetName()
    );

    //Nos aseguramos que no se llame de nuevo Acquire() cuando el
    //thread ya lo llamó (deadlock) 
    ASSERT(!(IsHeldByCurrentThread()));

    mutex->P();
    Current = currentThread;
}

void
Lock::Release()
{
    DEBUG('t', "Hago Release en el Lock %s, soy %s\n",
        this->GetName(),
        currentThread->GetName()
    );

    //Solo el mismo thread que llamó Acquire puede hacer Release()
    ASSERT(IsHeldByCurrentThread());

    Current = NULL;
    mutex->V();
}

bool
Lock::IsHeldByCurrentThread() const
{
    return  (Current == currentThread);
}
