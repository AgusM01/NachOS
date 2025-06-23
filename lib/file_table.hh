#ifndef __FILE_TABLE_HH__
#define __FILE_TABLE_HH__
#include "list.hh"
#include "filesys/open_file.hh"
#include <cstring>

// Una FileTable es una tabla que asocia cada archivo con un int que indica 
// la cantidad de threads que tienen abiertos dicho archivo.

struct fileStruct {
    OpenFile* file; // Archivo.
    const char *name; // Nombre del archivo.
    int open; // Cuantos procesos tienen abierto el archivo.
    bool deleted; // Indica si el archivo fué eliminado.
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
    
    // Checkea si un archivo está en la tabla.
    // Si está devuelve el índice.
    // Si no, -1.
    int CheckFileInTable(const char *name);
    
    // Devuelve la cantidad de procesos que tienen abierto
    // un archivo.
    // Si el archivo no está, devuelve -1;
    int GetOpen(const char *name);
    
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
    
