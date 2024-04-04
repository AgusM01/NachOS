/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_prod_cons_lock.hh"
#include "system.hh"
#include "lock.hh"
#include "condition.hh"
#include <stdio.h>

#define BUFSIZE 10

int itemCountLock = 0;
Lock mutexLock = Lock("mutex 1");

void producer_lock(void *name_){
    while(1){
        mutexLock.Acquire();
        if (itemCountLock < BUFSIZE){
            currentThread->Yield();
            itemCountLock++;
            currentThread->Yield();
        }
        mutexLock.Release();
        if (itemCountLock > 10)
            printf("Soy productor, itemCount: %d\n", itemCountLock);
        currentThread->Yield();
    }
}

void consumer_lock(void *name_){
    while(1){
        mutexLock.Acquire();
        if (itemCountLock > 0){
            currentThread->Yield();
            itemCountLock--;
            currentThread->Yield();
        }
        mutexLock.Release();
        if (itemCountLock < 0)
            printf("Soy consumidor, itemCount: %d\n", itemCountLock);
        currentThread->Yield();
    }
}
void
ThreadTestProdConsLock()
{
   const char* n[10] = {"t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9", "t10"};
    for (int i = 0; i < 10; i++){
        Thread *newThread = new Thread(n[i]); 
        if (i % 2){
            newThread->Fork(producer_lock, NULL);
        }
        else{
            newThread->Fork(consumer_lock, NULL);
        }
    }

    while(1){
        currentThread->Yield();
    }
    return;
}
