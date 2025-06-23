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
#include "semaphore.hh"
#include "system.hh"
#include <stdlib.h>


/// Dummy functions -- so we can compile our later assignments.
///

Condition::Condition(const char *debugName, Lock *conditionLock)
{
    queue = new List<Semaphore*>;
    condLock = conditionLock;
    name = debugName;
}

Condition::~Condition()
{
    Semaphore* tbd = nullptr;
    while ((tbd = queue->Pop()) != nullptr) {
        delete tbd;
    }
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
    DEBUG('s', "Hago Wait en %s, soy %s\n",
        this->GetName(),
        currentThread->GetName()
    );

    ASSERT(condLock->IsHeldByCurrentThread());

    char* semName = this->name ? concat("SemCond ", this->name) : nullptr;

    Semaphore* stop = new Semaphore(semName,0);

    queue->Append(stop);
    condLock->Release();

    stop->P(); // Si pasa de acá es porque recibió un signal.
    delete stop;
    free(semName);
    condLock->Acquire();
}

void
Condition::Signal()
{   
    DEBUG('s', "Hago Signal en %s, soy %s\n",
        this->GetName(),
        currentThread->GetName()
    );

    ASSERT(condLock->IsHeldByCurrentThread());

    Semaphore* sem;
    if ((sem = queue->Pop()) != nullptr)
        sem->V();
}

void
Condition::Broadcast()
{
    DEBUG('s', "Hago Broadcast en %s., soy %s\n",
        this->GetName(),
        currentThread->GetName()
    );

    ASSERT(condLock->IsHeldByCurrentThread());
    Semaphore* sem;

    while ((sem = queue->Pop()) != nullptr)
        sem->V();

}
