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


#include "condition.hh"
#include "system.hh"


/// Dummy functions -- so we can compile our later assignments.
///

Condition::Condition(const char *debugName, Lock *conditionLock)
{
    queue = new Semaphore(nullptr, 0);
    condLock = conditionLock;
    cont = 0;
    name = debugName;
}

Condition::~Condition()
{
    delete queue;
}

const char *
Condition::GetName() const
{
    return name;
}

void
Condition::Wait() /*Preguntar atomicidad*/
{
    DEBUG('t', "Hago Wait en Condition %s, soy %s\n",
        this->GetName(),
        currentThread->GetName()
    );

    ASSERT(condLock->IsHeldByCurrentThread());

    cont++; // NO necesita mutex, Wait se llama con el mutex ganado.
    condLock->Release();
    queue->P();
    condLock->Acquire();
}

void
Condition::Signal()
{   
    DEBUG('t', "Hago Signal en Condition %s, soy %s\n",
        this->GetName(),
        currentThread->GetName()
    );

    ASSERT(condLock->IsHeldByCurrentThread());

    if (cont > 0) {
        queue->V();
        cont--; // NO necesita mutex, Signal se llama con el mutex ganado.
    }
}

void
Condition::Broadcast()
{
    DEBUG('t', "Hago Broadcast en Condition %s, soy %s\n",
        this->GetName(),
        currentThread->GetName()
    );

    ASSERT(condLock->IsHeldByCurrentThread());

    while (cont > 0){
        queue->V();
        cont--; // NO necesita mutex, Broadcast se llama con el mutex ganado.
    }
        
}
