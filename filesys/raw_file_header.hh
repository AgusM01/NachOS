/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_FILESYS_RAWFILEHEADER__HH
#define NACHOS_FILESYS_RAWFILEHEADER__HH


#include "machine/disk.hh"

static const unsigned NUM_INDIRECT
  = (SECTOR_SIZE - 2 * sizeof (int)) / sizeof (int); // Numero de punteros que entran en un sector.


//const unsigned MAX_FILE_SIZE = NUM_DIRECT * SECTOR_SIZE; // El tamaño máximo de un archivo es 3840 Bytes.

// Utilizadas para el ejercicio 2 de FileSystem.
static const unsigned MAGIC_SIZE = sizeof (int);
static const unsigned DISK_SIZE = MAGIC_SIZE + NUM_SECTORS * SECTOR_SIZE;
const unsigned MAX_FILE_SIZE = DISK_SIZE;

struct RawFileHeader {
    unsigned numBytes;  ///< Number of bytes in the file.
    unsigned numSectors;  ///< Number of data sectors in the file.
    
    // Esto es lo máximo que puede almacenar el fileHeader.
    // Si se quiere aumentar se deben hacer NUM_DIRECT pointers
    // a más sectores.
    //unsigned dataSectors[NUM_DIRECT];  ///< Disk sector numbers for each data
                                       ///< block in the file.
                                       /// Tiene solo 1 nivel de indirección.
                                       /// Se debe aumentar con doble indirección.
    // Para dos indirecciones.
    unsigned dataSectors[NUM_INDIRECT];
};

#endif
