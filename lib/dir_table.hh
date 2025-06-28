#ifndef __DIR_TABLE_HH__
#define __DIR_TABLE_HH__

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

// Una DirTable es una tabla que va llevando los directorios presentes 
// en el sistema junto con metadata de los mismos útiles para su 
// utilización y seguridad.

struct dirStruct {
    OpenFile* file; // Archivo que contiene al directorio.
    char* actName; // Nombre del directorio.
    Lock* actDirLock; // Lock para actualizar el directorio.
    char* fatherName; // Nombre del directorio de este directorio.
                      // Sirve para Ej4.
                      // Puede ser nullptr en caso que sea root.
    int numEntries;   // Cantidad de entradas que tiene.       
    // Posiblemente se deban agregar más locks y quizás
    // algún array que indique que directorios hijos tiene,
    // así como también qué directorio padre.
};

class DirTable {
    public:

        // 20 directorios como máximo.
        static const unsigned SIZE = 20;

        // Constructor de la DirTable.
        DirTable();

        // Añade un archivo a la DirTable.
        // Toma:
        // * 'file': El archivo que contiene al directorio.
        // * 'actName': Nombre del directorio actual.
        // * 'fatherName': Nombre del padre del directorio.
        //                 Si es nullptr es porque el directorio
        //                 es el root.
        int Add(OpenFile* file, const char* actName, const char* fatherName);
        
        // Devuelve el directorio buscándolo por su nombre.
        // En caso de no estar, devuelve nullptr.
        OpenFile* GetDir(const char *name);

        // Checkea si el directorio con dicho nombre 
        // existe.
        // De no existir, devuelve -1.
        int CheckDirInTable(const char* name);
        
        // Devuelve la cantidad de entradas de un directorio.
        // Falla si dicho directorio no existe.
        int GetNumEntries(const char* name);

        // Setea la cantidad de entradas de un directorio.
        // Devuelve la cantidad de entradas resultantes.
        // Falla si el directorio no existe.
        int SetNumEntries(const char* name, int numEntries);

        // Realiza una operación con el lock del directorio
        // ingresado.
        // Devuelve true si sale todo bien, false en caso contrario.
        bool DirLock(const char *name, int op);

private:
        
    // Elementos de la tabla
    dirStruct data[SIZE];
    
    // El índice actual para añadir un nuevo ítem.
    int current;
    
    // Una lista que mantiene los índices de los elementos que fueron liberados
    // y no están entre los que tienen números altos, por lo que no es posible
    // modificar "current". En otras palabras, esto maneja la fragmentación externa.
    List<int> freed;
};

#endif /*__DIR_TABLE_HH__*/
