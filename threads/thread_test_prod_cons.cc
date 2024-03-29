/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_prod_cons.hh"
#include "system.hh"
#include "lock.hh"
#include <stdio.h>

#define BUFSIZE 10

int itemCount = 0;
Lock mutex = Lock("mutex 1"); 

void producer(void *name_){
    while(1){
        if (itemCount < BUFSIZE){
            mutex.Acquire();
            currentThread->Yield();
            itemCount++;
            currentThread->Yield();
            mutex.Release();
        }
        if (itemCount > 10)
            printf("Soy productor, itemCount: %d\n", itemCount);
        currentThread->Yield();
    }
}

void consumer(void *name_){
    while(1){
        if (itemCount > 0){
            currentThread->Yield();
            mutex.Acquire();
            itemCount--;
            currentThread->Yield();
            mutex.Release();
        }
        if (itemCount < 0)
            printf("Soy consumidor, itemCount: %d\n", itemCount);
        currentThread->Yield();
    }
}
void
ThreadTestProdCons()
{
   const char* n[10] = {"t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9", "t10"};
    for (int i = 0; i < 10; i++){
        Thread *newThread = new Thread(n[i]); 
        if (i % 2){
            newThread->Fork(producer, NULL);
        }
        else{
            newThread->Fork(consumer, NULL);
        }
    }

    while(1){
        currentThread->Yield();
    }
    return;
}

