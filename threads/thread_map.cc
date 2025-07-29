#include "threads/thread_map.hh"

ThreadMap::ThreadMap() {
    table = new Table<Thread*>();
    indexList = new List<int>();
}

ThreadMap::~ThreadMap() {
    delete indexList;
    delete table;
}

int 
ThreadMap::Add(Thread* t)
{
    int pid_proc = table->Add(t);

    ASSERT(pid_proc != -1);

    indexList->Append(pid_proc);
    return pid_proc;
}

Thread*
ThreadMap::Get(int pid_p)
{
    Thread* t;
    t = table->Get(pid_p);
    return t;
}

Thread* ThreadMap::Remove(int pid_p)
{
    Thread* t;
    indexList->Remove(pid_p);
    t = table->Remove(pid_p);
    return t;
}

bool
ThreadMap::IsEmpty()
{
    return table->IsEmpty();
}

void
ThreadMap::DelThreads(){
    while(!IsEmpty()){
        int i = indexList->Pop();
        Thread* t = table->Remove(i);
        delete t;
    }
}