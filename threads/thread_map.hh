/// All synchronization objects have a `name` parameter in the constructor;
/// its only aim is to ease debugging the program.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2000      José Miguel Santos Espino - ULPGC.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.
//
#ifndef NACHOS_THREADS_MAP__HH
#define NACHOS_THREADS_MAP__HH

#define SEMAPHORE_TEST

#include "lib/table.hh"
#include "lib/list.hh"
#include "threads/thread.hh"
#include "threads/lock.hh"

class Thread;

class ThreadMap {
public:

    ThreadMap();

    ~ThreadMap();

    int Add(Thread* t);

    Thread* Get(int pid_p);

    Thread* Remove(int pid_p);

    bool IsEmpty();

    void DelThreads();
private:

    /// For debugging.
    const char *name;

    Table<Thread*> *table;

    /// Queue of threads waiting on `P` because the value is zero.
    List<int> *indexList;

};


#endif
