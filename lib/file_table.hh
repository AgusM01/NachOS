#ifndef __FILE_TABLE_HH__
#define __FILE_TABLE_HH__
#include "threads/condition.hh"
#include "list.hh"
#include "filesys/open_file.hh"
#include <cstring>

#define ACQUIRE 0
#define RELEASE 1

#define SEM_P 0
#define SEM_V 1

#define WAIT 0
#define SIGNAL 1
#define BROADCAST 2

// Una FileTable es una tabla que asocia cada archivo con un int que indica 
// la cantidad de threads que tienen abiertos dicho archivo.

struct fileStruct {
    OpenFile* file; // Archivo.
    char *name; // Nombre del archivo.
    int open; // Cuantos procesos tienen abierto el archivo.
    bool deleted; // Indica si el archivo fué eliminado.
    int readers; // Cantidad de lectores actuales del archivo.
    bool writer; // Si hay un escritor queriendo escribir.
    bool closed; // Si el archivo fué cerrado.
    Lock *OpenRemoveLock; // Lock para abrir y eliminar el archivo.
    Condition *RemoveCondition; // Condición para eliminar el archivo cuando todos lo tengan cerrado.
    Condition *WriterCondition; // Condición para escribir un archivo cuando nadie esté leyendolo.
    Semaphore *ReadersSem;
    Lock *RdWrLock; // Condición para escribir en el archivo sincronizando lector/escritor.
    Lock *WrLock; // Condición para escribir en el archivo sincronizando escritores.
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
    int Add(OpenFile* file, const char *name);

    // Checkea si un índice dado tiene un archivo asociado.
    bool HasKey(int i) const;

    // Devuelve el archivo asociado a dicho índice.
    OpenFile* GetFileIdx(int i);

    // Devuelve el archivo buscándolo por su nombre.
    OpenFile* GetFile(const char *name);

    // Devuelve la lista de Pids asociada a dicho índice.
    unsigned* GetPids(int i);
    
    // Devuelve la cantidad de lectores del archivo.
    int GetReaders(const char *name);

    // Añade un lector al archivo. 
    // Es región crítica por lo que se accede mediante el WriterLock.
    int AddReader(const char *name);

    // Elimina un lector del archivo. 
    // Es región crítica por lo que se accede mediante el WriterLock.
    int RemoveReader(const char *name);
    
    // Devuelve si hay un escritor queriendo escribir.
    bool GetWriter(const char* name);

    // Setea un escritor.
    int SetWriter(const char* name, bool w);

    // Checkea si un archivo está en la tabla.
    // Si está devuelve el índice.
    // Si no, -1.
    int CheckFileInTable(const char *name);
    
    // Devuelve la cantidad de procesos que tienen abierto
    // un archivo.
    // Si el archivo no está, devuelve -1;
    int GetOpen(const char *name);

    // Retorna si el archivo fué cerrado o no.
    bool GetClosed(const char *name);

    // Setea el elemento cerrado del archivo.
    int SetClosed(const char *name, bool v);
    
    // Un thread cierra un archivo.
    // Reduce el número que lo tienen abierto.
    // En caso de éxito, retorna la cantidad de hilos
    // que mantienen el archivo abierto.
    // En caso de error, retorna -1.
    int CloseOne(const char *name);

    // Un thread elimina el archivo.
    // Hay que marcarlo como eliminado para que no se pueda abrir por otro thread.
    int Delete(const char *name);
    
    // Devuelve si un archivo ha sido borrado o no.
    bool isDeleted(const char *name);

    // Elimina el archivo de la tabla.
    // Devuelve el índice del archivo si todo sale bien.
    // De lo contrario, devuelve -1.
    int Remove(const char *name);

    // Realiza la operación indicada con el lock
    // utilizado para abrir o eliminar un archivo.
    bool FileORLock(const char *name, int op);

    // Reliza la operación indicada con la condición
    // utilizada al momento de eliminar un archivo.
    bool FileRemoveCondition(const char *name, int op);
    
    // Reliza la operación indicada con la condición
    // utilizada al momento de escribir un archivo.
    bool FileWriterCondition(const char *name, int op);

    // Realiza la operación indicada con el semáforo
    // utilizado para 
    bool FileSem(const char *name, int op);

    // Realiza la operación indicada con el lock
    // utilizado para escribir en el archivo.
    // Sincronizando entre lectores - escritor.
    bool FileWrLock(const char *name, int op);

    // Realiza la operación indicada con el lock
    // utilizado para escribir en el archivo.
    // Sincronizando entre escritores.
    bool FileRdWrLock(const char *name, int op);

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


#endif // __FILE_TABLE_HH__
    
