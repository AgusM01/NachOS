#include "dir_table.hh"
#include "filesys/directory_entry.hh"
#include "lib/utility.hh"
#include "threads/system.hh"

#include <cstring>

DirTable::DirTable()
{
    current = 0;
}

int 
DirTable::Add(OpenFile* file, const char* actName, const char* fatherName)
{
    ASSERT(file != nullptr);
    ASSERT(actName != nullptr);

    int i = CheckDirInTable(actName);
    if (i != -1)
    {
        DEBUG('f', "El directorio %s ya está en la DirTable\n", actName);
        // Ver si no hay que hacer algo como con los archivos cerrados.
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
    
    DEBUG('f', "Añado el directorio %s a la DirTable\n", actName);
    
    data[cur_ret].file = file;
    
    ASSERT(strlen(actName) <= DIR_NAME_MAX_LEN); 
    data[cur_ret].actName = new char[DIR_NAME_MAX_LEN];
    strcpy(data[cur_ret].actName, actName);

    if (fatherName != nullptr){
        DEBUG('f', "El directorio %s tiene como padre al directorio %s\n", actName, fatherName);
        ASSERT(strlen(fatherName) <= DIR_NAME_MAX_LEN); 
        data[cur_ret].fatherName = new char[DIR_NAME_MAX_LEN];
        strcpy(data[cur_ret].fatherName, fatherName);
    }
    else
        DEBUG('f', "El directorio %s no tiene un padre, por lo tanto es root\n", actName);

    // Empieza con 0 entradas.
    // Después se van agregando.
    data[cur_ret].numEntries = 0;
    
    char* actDirLockName = concat("DirLock.", actName);
    char* removeConditionName = concat("RemoveCondition.",actName);
    data[cur_ret].actDirLock = new Lock(actDirLockName);
    data[cur_ret].RemoveCondition = new Condition(removeConditionName, data[cur_ret].actDirLock);
    
    // Cada vez que se haga un Cd tiene que sumar 1.
    // Cuando sale, restar 1.
    data[cur_ret].threadsInIt = 0;
    data[cur_ret].toDelete = false;
    
    numCondition = 0;

    return cur_ret;
}

OpenFile*
DirTable::GetDir(const char* name)
{
    int idx = CheckDirInTable(name);
    
    if (idx == -1){
        DEBUG('f', "El directorio %s no se encuentra en la DirTable\n", name);
        return nullptr;
    }

    return data[idx].file;
}

int 
DirTable::CheckDirInTable(const char* name)
{
   for (int i = 0; i < current; i++)
       if (!strcmp(name, data[i].actName)){
            return i;
       }
   
   return -1;
}

int
DirTable::GetNumEntries(const char* name)
{
    int idx = CheckDirInTable(name);
    ASSERT(idx != -1);
    
    return data[idx].numEntries;
}

int
DirTable::SetNumEntries(const char* name, int numEntries)
{
    int idx = CheckDirInTable(name);
    ASSERT(idx != -1);

    data[idx].numEntries = numEntries;
    return numEntries;
}

int 
DirTable::setToDelete(const char* name)
{
    int idx = CheckDirInTable(name);

    ASSERT(idx != -1);
    
    data[idx].toDelete = true;
    return 0;

}

bool 
DirTable::getToDelete(const char* name)
{
    int idx = CheckDirInTable(name);

    ASSERT(idx != -1);
    
    return data[idx].toDelete;
}


bool 
DirTable::DirLock(const char *name, int op)
{
    int idx = CheckDirInTable(name);

    ASSERT(idx != -1);

    switch (op) { 
        case ACQUIRE:
            DEBUG('f', "Soy %d y tomo el DirLock del directorio %s en la DirTable\n",currentThread->GetPid(), name);
            (data[idx].actDirLock)->Acquire();
        break;

        case RELEASE:
            DEBUG('f', "Soy %d y suelto el DirLock del directorio %s en la DirTable\n",currentThread->GetPid(), name);
            (data[idx].actDirLock)->Release();
        break;

        default:
            return false;
        break;
    }

    return true;
}

int
DirTable::addThreadsIn(const char* name)
{
    int idx = CheckDirInTable(name);
    
    if (idx == -1){
        DEBUG('f', "El directorio %s no forma parte de la DirTable\n",name);
        return -1;
    }

    data[idx].threadsInIt += 1;
    return data[idx].threadsInIt;
}

int 
DirTable::removeThreadsIn(const char* name)
{
    int idx = CheckDirInTable(name);
    
    if (idx == -1){
        DEBUG('f', "El directorio %s no forma parte de la DirTable\n",name);
        return -1;
    }
    
    unsigned actual = data[idx].threadsInIt;
    data[idx].threadsInIt = actual == 0 ? 0 : actual - 1;
    return data[idx].threadsInIt;
}

int
DirTable::getThreadsIn(const char* name)
{
    int idx = CheckDirInTable(name);

    if (idx == -1){
        DEBUG('f', "El directorio %s no forma parte de la DirTable\n",name);
        return -1;
    }

    return data[idx].threadsInIt;
}

// En caso de haber hecho Wait,
// sale de la función con el lock adquirido.
bool 
DirTable::DirRemoveCondition(const char *name, int op)
{
    int idx = CheckDirInTable(name);

    ASSERT(idx != -1);

    switch (op) { 
        case WAIT:
            DEBUG('f', "Soy %d y hago wait sobre la DirRemoveCondition del directorio %s\n",currentThread->GetPid(), name);
            (data[idx].RemoveCondition)->Wait();
        break;

        case SIGNAL:
            DEBUG('f', "Soy %d y hago signal sobre la DirRemoveCondition del directorio %s\n",currentThread->GetPid(), name);
                (data[idx].RemoveCondition)->Signal();
        break;
    
        case BROADCAST:
            DEBUG('f', "Hago broadcast sobre la DirRemoveCondition del directorio %s\n", name);
            (data[idx].RemoveCondition)->Broadcast();
        break;

        default:
            return false;
        break;
    }
    return true;
}
