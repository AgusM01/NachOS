#include "file_table.hh"
#include "filesys/directory_entry.hh"
#include "lib/utility.hh"

#include <cstring>

FileTable::FileTable()
{
    current = 0;
}

int
FileTable::Add(OpenFile* file, const char *name)
{

    int i = CheckFileInTable(name);

    if (i != -1)
    {
        data[i].open += 1;
        return i;
    }
    
    int cur_ret;

    if (!freed.IsEmpty())
        cur_ret = freed.Pop();
    else
    {
        if (current < static_cast<int>(SIZE)){
            cur_ret = current;
            current++;
        }
        else 
            return -1;
    }   
    
    data[cur_ret].file = file;
    data[cur_ret].open = 1;
    data[cur_ret].readers = 0;
    data[cur_ret].writer = false;

    data[cur_ret].name = new char[FILE_NAME_MAX_LEN];
    strcpy(data[cur_ret].name, name);
    
    data[cur_ret].deleted = false;

    char* openRemoveLockName = concat("FTOpenRemoveLock.", name);
    char* conditionRemoveName = concat("FTConditionRemove.", name);
    char* conditionWriteName = concat("FTConditionWrite.", name);
    char* rdwrLockName = concat("FTRdWrLock.", name);
    char* wrLockName = concat("FTWrLock.", name);
    char* readersSemName = concat("FTReaderSem.", name);

    data[cur_ret].OpenRemoveLock = new Lock(openRemoveLockName);
    data[cur_ret].RdWrLock = new Lock(rdwrLockName);
    data[cur_ret].WrLock = new Lock(wrLockName);
    data[cur_ret].RemoveCondition = new Condition(conditionRemoveName, data[cur_ret].OpenRemoveLock);
    data[cur_ret].WriterCondition = new Condition(conditionWriteName, data[cur_ret].RdWrLock);
    data[cur_ret].ReadersSem = new Semaphore(readersSemName, 0);
    return cur_ret;
}


OpenFile* 
FileTable::GetFileIdx(int i)
{
    ASSERT(i >= 0);

    return HasKey(i) ? data[i].file : nullptr;
}

OpenFile*
FileTable::GetFile(const char *name)
{
    int idx = CheckFileInTable(name);
    
    return idx == -1 ? nullptr : data[idx].file;
}

int 
FileTable::CheckFileInTable(const char *name)
{
   for (int i = 0; i < current; i++)
       if (!strcmp(name, data[i].name))
           return i;

   return -1;
}

bool
FileTable::HasKey(int i) const
{
    ASSERT(i >= 0);

    return i < current && !freed.Has(i);
}

int
FileTable::GetReaders(const char *name)
{
    int idx = CheckFileInTable(name);
    return idx == -1 ? -1 : data[idx].readers;
}

int
FileTable::AddReader(const char* name)
{
    int idx = CheckFileInTable(name);
    
    if (idx == -1)
        return -1;
    
    data[idx].readers += 1;

    return data[idx].readers;
}

int
FileTable::RemoveReader(const char* name)
{
    int idx = CheckFileInTable(name);
    
    if (idx == -1 || data[idx].readers == 0)
        return -1;

    data[idx].readers -= 1;
    return data[idx].readers;
}

bool
FileTable::GetWriter(const char *name)
{
    int idx = CheckFileInTable(name);
    
    ASSERT(idx != -1);

    return data[idx].writer;
}

int 
FileTable::SetWriter(const char *name, bool w)
{
    int idx = CheckFileInTable(name);

    return idx == -1 ? -1 : data[idx].writer = w;
}

int 
FileTable::GetOpen(const char *name)
{
    int idx = CheckFileInTable(name);

    return idx == -1 ? -1 : data[idx].open;
}

int
FileTable::CloseOne(const char *name)
{
    int idx = CheckFileInTable(name);
    
    ASSERT(idx != -1);
    
    if (data[idx].open == 1)
        return -1;

    data[idx].open -= 1;
    return data[idx].open;
}

int
FileTable::Delete(const char *name)
{
    int idx = CheckFileInTable(name);

    ASSERT(idx != -1);

    data[idx].deleted = true;
    return idx;
}

bool
FileTable::isDeleted(const char *name)
{
    int idx = CheckFileInTable(name);

    ASSERT(idx != -1);
    
    return data[idx].deleted;
}

int 
FileTable::Remove(const char *name)
{
    int i = CheckFileInTable(name);

    if (i == -1)
        return -1;
    
   // if (data[i].open > 1){
   //     data[i].open -= 1;
   //     return i;
   // }

    if (i == current - 1) {
        current--;
        for (int j = current - 1; j >= 0 && !HasKey(j); j--) {
            ASSERT(freed.Has(j));
            freed.Remove(j);
            current--;
        }
    } else {
        freed.SortedInsert(i, i);
    }

    delete data[i].name;
    delete data[i].ReadersSem;
    delete data[i].RemoveCondition;
    delete data[i].WriterCondition;
    delete data[i].WrLock;
    delete data[i].RdWrLock;
    delete data[i].OpenRemoveLock;
    
    return i;
}

bool 
FileTable::FileORLock(const char *name, int op)
{
    int idx = CheckFileInTable(name);

    ASSERT(idx != -1);

    switch (op) { 
        case ACQUIRE:
            (data[idx].OpenRemoveLock)->Acquire();
        break;

        case RELEASE:
            (data[idx].OpenRemoveLock)->Release();
        break;

        default:
            return false;
        break;
    }

    return true;
}

bool 
FileTable::FileWrLock(const char *name, int op)
{
    int idx = CheckFileInTable(name);

    ASSERT(idx != -1);

    switch (op) { 
        case ACQUIRE:
            (data[idx].WrLock)->Acquire();
        break;

        case RELEASE:
            (data[idx].WrLock)->Release();
        break;

        default:
            return false;
        break;
    }

    return true;
}

bool 
FileTable::FileRdWrLock(const char *name, int op)
{
    int idx = CheckFileInTable(name);

    ASSERT(idx != -1);

    switch (op) { 
        case ACQUIRE:
            (data[idx].RdWrLock)->Acquire();
        break;

        case RELEASE:
            (data[idx].RdWrLock)->Release();
        break;

        default:
            return false;
        break;
    }

    return true;
}

// En caso de haber hecho Wait,
// sale de la función con el lock adquirido.
bool 
FileTable::FileRemoveCondition(const char *name, int op)
{
    int idx = CheckFileInTable(name);

    ASSERT(idx != -1);

    switch (op) { 
        case WAIT:
            (data[idx].RemoveCondition)->Wait();
        break;

        case SIGNAL:
            (data[idx].RemoveCondition)->Signal();
        break;
    
        case BROADCAST:
            (data[idx].RemoveCondition)->Broadcast();
        break;

        default:
            return false;
        break;
    }

    return true;
}

// En caso de haber hecho Wait,
// sale de la función con el lock adquirido.
bool 
FileTable::FileWriterCondition(const char *name, int op)
{
    int idx = CheckFileInTable(name);

    ASSERT(idx != -1);

    switch (op) { 
        case WAIT:
            (data[idx].WriterCondition)->Wait();
        break;

        case SIGNAL:
            (data[idx].WriterCondition)->Signal();
        break;
    
        case BROADCAST:
            (data[idx].WriterCondition)->Broadcast();
        break;

        default:
            return false;
        break;
    }

    return true;
}

bool 
FileTable::FileSem(const char *name, int op)
{
    int idx = CheckFileInTable(name);

    ASSERT(idx != -1);

    switch (op) { 
        case SEM_P:
            (data[idx].ReadersSem)->P();
        break;

        case SEM_V:
            (data[idx].ReadersSem)->V();
        break;

        default:
            return false;
        break;
    }

    return true;
}
