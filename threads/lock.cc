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
#include "thread.hh"
#include <limits>
#include <stdio.h>
#include <stdlib.h>

/// Dummy functions -- so we can compile our later assignments.

Lock::Lock(const char *debugName)
{
    
    semName = debugName ? concat("JoinSem ", debugName) : nullptr;
    mutex = new Semaphore(semName, 1);
    name = debugName;
    holder = NULL;
    holderOriginalPriority = -1;
}

Lock::~Lock()
{
    /*el mutex creado por new en constructor.*/
    delete mutex;
    free(semName);
}

const char *
Lock::GetName() const
{
    return name;
}

void
Lock::Acquire()
{
    DEBUG('s', "Hago Acquire en el %s, soy %s\n",
        this->GetName(),
        currentThread->GetName()
    );

    //Nos aseguramos que no se llame de nuevo Acquire() cuando el
    //thread ya lo llamó (deadlock) 
    ASSERT(!(IsHeldByCurrentThread()));

    //Inversión de prioridades
    //Si Current tiene menos priorida que el currentThread,
    //Le damos a Current la misma prioridad que currentThread
    //para que pueda soltar el lock más rápido, dando, valga
    //la redundancia, mas prioridad al thread con más prioridad
    // -------------------- Explicación 5 ------------------------
    //Esto no se puede implemetar en semáforos, ya que no tienen la
    //misma condición de los locks, donde solo el thread que hace un Acquire
    //puede hacer un Release.
    //Es decir, no necesariamente el Thread va a quedar bloqueado en
    //un P gracias a un solo Thread, por lo que no hay criterio para
    //cambiar las prioridades. 
    CheckPriority();

    mutex->P();
    holder = currentThread;
    holderOriginalPriority = currentThread->GetPriority();
}

void
Lock::Release()
{
    DEBUG('s', "Hago Release en el %s, soy %s\n",
        this->GetName(),
        currentThread->GetName()
    );

    //Solo el mismo thread que llamó Acquire puede hacer Release()
    ASSERT(IsHeldByCurrentThread());


    if (holder->GetPriority() != holderOriginalPriority)
        holder->SetPriority(holderOriginalPriority);
    holder = NULL;
    mutex->V();
}

bool
Lock::IsHeldByCurrentThread() const
{
    return  (holder == currentThread);
}

void
Lock::CheckPriority()
{
    int ctp; //current thread priority


    if (holder != nullptr && holder->GetPriority() < (ctp = currentThread->GetPriority())){

        DEBUG('z', "Cambio de prioridad, thread %s de prioridad %d a %d\n",
            holder->GetName(),
            holder->GetPriority(),
            ctp
        );

        DEBUG('z',"Status de %s es %s\n", holder->GetName(), holder->PrintStatus());

        if (holder->GetStatus() == READY)
            scheduler->ChangePriority(holder, ctp);

        holder->SetPriority(ctp);
    }
}