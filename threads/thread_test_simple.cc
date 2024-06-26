/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_simple.hh"
#include "system.hh"

#include <stdio.h>
#include <string.h>

//#include "semaphore.hh"


/// Loop 10 times, yielding the CPU to another ready thread each iteration.
///
/// * `name` points to a string with a thread name, just for debugging
///   purposes.

bool threadsDone[4] = {false};

#ifdef SEMAPHORE_TEST
Semaphore s = Semaphore(NULL,3);
#endif

void
SimpleThread(void *name_)
{
    // If the lines dealing with interrupts are commented, the code will
    // behave incorrectly, because printf execution may cause race
    // conditions.
    for (unsigned num = 0; num < 10; num++) {
        
        #ifdef SEMAPHORE_TEST
        s.P();
        #endif

        printf("*** Thread `%s` is running: iteration %u\n", currentThread->GetName(), num);
                
        #ifdef SEMAPHORE_TEST
        s.V();
        #endif
        currentThread->Yield();
    }
    threadsDone[currentThread->GetName()[0] - '2'] = true;

    printf("!!! Thread `%s` has finished SimpleThread\n", currentThread->GetName());
 
}

/// Set up a ping-pong between several threads.
///
/// Do it by launching one thread which calls `SimpleThread`, and finally
/// calling `SimpleThread` on the current thread.
void
ThreadTestSimple()
{
    const char* t[4] = {"2nd", "3rd", "4th", "5th"};
    for (int i = 0; i < 4; i++){
        Thread *newThread = new Thread(t[i],false);
        newThread->Fork(SimpleThread, NULL);
    }
    
    //the "main" thread also executes the same function
    SimpleThread(NULL);

   //Wait for the 2nd thread to finish if needed
    while (!(threadsDone[0] && 
             threadsDone[1] &&
             threadsDone[2] &&
             threadsDone[3])){
        currentThread->Yield(); 
    }
    printf("Test finished\n");
}
