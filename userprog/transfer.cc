/// Copyright (c) 2019-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


// Los programas en NACHOS se ejecutan en la máquina simulada.
// NACHOS se ejecuta sobre la máquina x86 actual (mi compu).
// Todas las direcciones utilizadas en el programa de usuario serán
// direcciones de la máquina simulada. Por lo tanto, no son válidas 
// en la memoria de la máquina donde se ejecuta NACHOS.

// En este archivo están las funciones para copiar datos desde el núcleo al espacio de memoria virtual del usuario y vicecersa.

#include "transfer.hh"
#include "lib/utility.hh"
#include "threads/system.hh"


void ReadBufferFromUser(int userAddress, char *outBuffer,
                        unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outBuffer != nullptr);
    ASSERT(byteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        ASSERT(machine->ReadMem(userAddress++, 1, &temp));
        *outBuffer = (unsigned char) temp;
        outBuffer++;
    } while (count < byteCount);

    return;
}

bool ReadStringFromUser(int userAddress, char *outString,
                        unsigned maxByteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outString != nullptr);
    ASSERT(maxByteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        ASSERT(machine->ReadMem(userAddress++, 1, &temp));
        *outString = (unsigned char) temp;
    } while (*outString++ != '\0' && count < maxByteCount);

    return *(outString - 1) == '\0';
}

void WriteBufferToUser(const char *buffer, int userAddress,
                       unsigned byteCount)
{   
    ASSERT(userAddress != 0);
    ASSERT(buffer != nullptr);
    ASSERT(byteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        temp = *((int*)buffer++);
        count++;
        ASSERT(machine->WriteMem(userAddress++, 1, temp));
    } while (count < byteCount);

    return;

}

void WriteStringToUser(const char *string, int userAddress)
{
    ASSERT(userAddress != 0);
    ASSERT(string != nullptr);

    do {
        int temp;
        temp = (*(int*)string++);
        ASSERT(machine->WriteMem(userAddress++, 1, temp));
    } while (*string != '\0');

    return;
}
