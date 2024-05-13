/// Consola sincronizada.
///
/// Esta interfaz representa una consola que no permite que dos threads escriban 
/// al mismo tiempo en la consola.
///
/// Sin embargo, si un hilo escribe, otro hilo PUEDE leer.

#ifndef NACHOS_MACHINE_SYNCHCONSOLE__HH
#define NACHOS_MACHINE_SYNCHCONSOLE__HH

#include "console.hh"
#include "threads/lock.hh"
#include "threads/semaphore.hh"

class SynchConsole {
public:
        
        /// Constructor de consola sincronizada.
        SynchConsole(const char *readFile, const char *writeFile,
            void *callArg);
        
        /// Destructor de consola sincronizada.
        ~SynchConsole();
        
        /// Escribir de forma sincronizada a la consola.
        void WriteSync(char ch);

        /// Leer de forma sincronizada a la consola.
        char ReadSync();

private:

        /// Lock para controlar la escritura.
        Lock* WriteMutex;

        /// Lock 
        /// para controlar la lectura.
        Lock* ReadMutex;
        
        /// Write Done
        Semaphore* writeDone;
        
        /// Read Avail
        Semaphore* readAvail;

        /// Consola 
        Console* cons;

        void ReadAvail(void *arg);

        void WriteDone(void *arg);

};

#endif  
