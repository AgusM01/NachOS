#include "list.hh"
#include "filesys/open_file.hh"

// Una FileTable es una tabla que asocia cada archivo con un int que indica 
// la cantidad de threads que tienen abiertos dicho archivo.

struct fileStruct {
    OpenFile* file;
    int open;
};

class FileTable {
public:

    // 50 archivos abiertos al mismo tiempo.
    // Se puede agrandar.
    static const unsigned SIZE = 50;

    // Construye una FileTable 
    FileTable();

    // Añade un archivo con el pid del thread el cual lo creó.
    // Si el archivo ya está en la tabla, se agrega el pid a
    // la lista de Pids;
    // Retora el índice donde agregó al archivo.
    // Retorna -1 si no hay espacio.
    int Add(OpenFile* file, unsigned pid);

    // Checkea si un índice dado tiene un archivo asociado.
    bool HasKey(int i) const;

    // Devuelve el archivo asociado a dicho índice.
    OpenFile* GetFile(int i); 

    // Devuelve la lista de Pids asociada a dicho índice.
    unsigned* GetPids(int i);
    
    // Checkea si un archivo está en la tabla.
    // Si está devuelve el índice.
    // Si no, -1.
    int CheckFileInTable(OpenFile* file);

    // Elimina el archivo de la tabla.
    // Si la cantidad de abiertos queda en 0, lo elimina de la tabla.
    // De lo contrario, reduce la cantidad en 1.
    // Devuelve el índice del archivo si todo sale bien.
    // De lo contrario, devuelve -1.
    int Remove(OpenFile* file);

private:

    // Elementos de la tabla.
    fileStruct data[SIZE];

    // El índice actual para añadir un nuevo ítem.
    int current;
    
    // Una lista que mantiene los índices de los elementos que fueron liberados
    // y no están entre los que tienen números altos, por lo que no es posible
    // modificar "current". En otras palabras, esto maneja la fragmentación externa.
    List<int> freed;
};

FileTable::FileTable()
{
    current = 0;
}

int
FileTable::Add(OpenFile* file, unsigned pid)
{

    int i = CheckFileInTable(file);

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
    
    return cur_ret;
}


OpenFile* 
FileTable::GetFile(int i)
{
    if(i > current)
        return nullptr;

    return data[i].file;
}


int 
FileTable::CheckFileInTable(OpenFile* file)
{
   for (int i = 0; i < current; i++)
       if (file == data[i].file)
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
FileTable::Remove(OpenFile* file)
{
    int i = CheckFileInTable(file);

    if (i == -1)
        return -1;
    
    if (data[i].open > 1){
        data[i].open -= 1;
        return i;
    }

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
    

    
