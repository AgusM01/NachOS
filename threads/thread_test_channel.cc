/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_channel.hh"
#include "condition.hh"
#include "semaphore.hh"
#include "system.hh"
#include "channel.hh"
#include <stdio.h>


Channel channel = Channel("Channel 1");
Semaphore barrierThreads = Semaphore("barrierThreads", 0);
Lock cvMutex = Lock("cvMutex");
Condition cv = Condition("cv", &cvMutex);
int counter = 0;


void sender(void *arg) {
    barrierThreads.P();
    barrierThreads.V();

    channel.Send(*((int*)arg));
}

void receiver(void *arg){
    barrierThreads.P();
    barrierThreads.V();

    int message = 0;

    channel.Receive(&message);

    cvMutex.Acquire();
    counter++;
    cv.Signal();
    cvMutex.Release();

    channel.Send(message);
}


void
ThreadTestChannel()
{
    const char* s[5] = {"sender1", "sender2","sender3","sender4","sender5"};
    const char* r[5] = {"receiver1", "receiver2","receiver3","receiver4","receiver5"};
    Thread *newThread[10];
    int args[5] = {1,2,3,4,5};

    for (int i = 0; i < 5; i++)
     newThread[i] = new Thread(s[i],false); 

    for (int i = 0; i < 5; i++)
     newThread[5 + i] = new Thread(r[i],false); 

    for (int i = 0; i < 5; i++)
        newThread[i]->Fork(sender, (void*)(args + i));

    for (int i = 0; i < 5; i++)
        newThread[5 + i]->Fork(receiver, nullptr);

    barrierThreads.V();

    cvMutex.Acquire();
    while(counter != 5) {
        cv.Wait();
    }
    cvMutex.Release();

    int message;

    for (int i = 0; i < 5; i++){
        channel.Receive(&message);
        printf("Soy main recibí %d\n", message);
        fflush(stdout);
    }

    return;
}

