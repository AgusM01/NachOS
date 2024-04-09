/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_prod_cons.hh"
#include "system.hh"
#include "lock.hh"
#include "condition.hh"
#include <stdio.h>

#define BUFSIZE 3
#define MAX 1000

int buf[BUFSIZE];
int posBuf = 0;

Lock mutexVc = Lock("mutex 1");
Condition cv = Condition(NULL, &mutexVc);

void producer(void *name_){
    for (int i = 0; i < MAX; i++){
        puts("por tomar mutex");
        mutexVc.Acquire();
        while (posBuf == 3){
            printf("Productor esperando (buffer lleno)\n");
            cv.Wait();
        }
        printf("Productor produce: %d en %d\n", i, posBuf);
        buf[posBuf] = i;
        posBuf++;
        if (posBuf == 1)
            cv.Signal();
        mutexVc.Release();
    }
}

void consumer(void *name_){
    for (int i = 0; i < MAX; i++){
        mutexVc.Acquire();
        if (posBuf == 0){
            puts("entro");
            cv.Signal();
        }
        while (posBuf == 0){
            printf("Consumidor esperando (buffer vacio)\n");
            cv.Wait();
        }
        printf("Consumidor consume: %d en %d\n", buf[posBuf], posBuf);
        posBuf--;
        
        mutexVc.Release();
    }
}
void
ThreadTestProdCons()
{
   const char* n[2] = {"P", "C"};
    for (int i = 0; i < 2; i++){
        Thread *newThread = new Thread(n[i]); 
        if (!i){
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

