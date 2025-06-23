#include "file_table.hh"

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
    data[cur_ret].name = name;
    data[cur_ret].deleted = false;
    
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
    return i;
}