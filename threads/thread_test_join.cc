/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_join.hh"
#include "semaphore.hh"
#include "system.hh"
#include "channel.hh"
#include "thread.hh"
#include <stdio.h>


Semaphore sj = Semaphore("sj", 0);

void
child(void* arg) {

    currentThread->Yield();
}

void
ThreadTestJoin()
{
    const char* s[5] = {"child1", "child2", "child3", "child4", "child5"};
    Thread *newThread[5];

    for(int i = 0; i < 5; i++)
        newThread[i] = new Thread(s[i], true, 5);

    for(int i = 0; i < 5; i++)
        newThread[i]->Fork(child, NULL);


    for(int i = 0; i < 5; i++){
        currentThread->Yield();
        newThread[i]->Join();
    }

    return;
}

