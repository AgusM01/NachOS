/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_prod_cons_vc.hh"
#include "system.hh"
#include "lock.hh"
#include "condition.hh"
#include <stdio.h>

#define BUFSIZE 10

int itemCountVc = 0;
Lock mutexVc = Lock("mutex 1");
Condition cv = Condition(NULL, &mutexVc);

void producer_vc(void *name_){
    while(1){
        mutexVc.Acquire();
        while (itemCountVc == BUFSIZE){
            cv.Wait();
        }
        currentThread->Yield();
        itemCountVc++;
        currentThread->Yield();
        cv.Signal();
        mutexVc.Release();
        if (itemCountVc > 10)
            printf("Soy productor, itemCount: %d\n", itemCountVc);
        currentThread->Yield();
    }
}

void consumer_vc(void *name_){
    while(1){
        mutexVc.Acquire();
        while (itemCountVc == 0){
            cv.Wait();
        }
        currentThread->Yield();
        itemCountVc--;
        currentThread->Yield();
        cv.Signal();
        mutexVc.Release();
        if (itemCountVc < 0)
            printf("Soy consumidor, itemCount: %d\n", itemCountVc);
        currentThread->Yield();
    }
}
void
ThreadTestProdConsVc()
{
   const char* n[10] = {"t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9", "t10"};
    for (int i = 0; i < 10; i++){
        Thread *newThread = new Thread(n[i]); 
        if (i % 2){
            newThread->Fork(producer_vc, NULL);
        }
        else{
            newThread->Fork(consumer_vc, NULL);
        }
    }

    while(1){
        currentThread->Yield();
    }
    return;
}

