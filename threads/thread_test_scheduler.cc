/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_scheduler.hh"
#include "condition.hh"
#include "semaphore.hh"
#include "system.hh"
#include "channel.hh"
#include "thread.hh"
#include <stdio.h>


Lock mx = Lock("mutex");
Condition cd = Condition("Cond", &mx);
int c = 0;

void
funcMasPriority(void* arg)
{
    mx.Acquire();
    currentThread->Yield();
    while( *((int*)arg) != c)
        cd.Wait();
    c++;
    cd.Broadcast();
    mx.Release();
}


void
ThreadTestScheduler()
{
    const char* s[10] = {"thread1", "thread2", "thread3", "thread4", "thread5", "thread6", "thread7", "thread8", "thread9", "thread10"};
    Thread *newThread[10];
    int args[10] = {0,1,2,3,4,5,6,7,8,9};


    for(int i = 0; i < 10; i++) {
        newThread[i] = new Thread(s[i], true, 9 - (i % 5));
    }


    for(int i = 0; i < 10; i++)
        newThread[i]->Fork(funcMasPriority, (void*)(args + i));


    for(int i = 0; i < 10; i++){
        newThread[i]->Join();
    }

    puts("Fin");

    return;
}

