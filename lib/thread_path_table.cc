#ifndef __THREAD_PATH_TABLE_HH__
#define __THREAD_PATH_TABLE_HH__

#include "threads/condition.hh"
#include "list.hh"
#include "filesys/open_file.hh"
#include <string>

#define MAX_PATH_LENGTH 50 

#define ACQUIRE 0
#define RELEASE 1

#define SEM_P 0
#define SEM_V 1

#define WAIT 0
#define SIGNAL 1
#define BROADCAST 2

// Una ThreadPathTable es una tabla que asocia cada hilo con
// un path sobre el cuál está trabajando.
// El path es un array strings donde el directorio i es padre del
// directorio i+1.


struct pthStruct {
    unsigned pid; // Pid del thread.
    char* path[50]; // Path del thread.
};

class ThreadPathTable{
    public:
        
        // Máximo 20 hilos.
        static const unsigned SIZE = 20;

        // Constructor de la pathtable.
        // Debe crearse junto con el NachOS.
        ThreadPathTable();

        // Todos los hilos empiezan en root (".").
        int Add(unsigned pid);

        // Checkea si un thread está en la tabla.
        // Devuelve el idx si está, de lo contrario
        // devuelve -1.
        int CheckThreadInTable(unsigned pid);
        
        int Cd(char** path);



}


#endif /*__THREAD_PATH_TABLE_HH__*/
