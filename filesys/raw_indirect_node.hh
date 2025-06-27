#ifndef NACHOS_FILESYS_RAWINDIRECTNODE__HH
#define NACHOS_FILESYS_RAWINDIRECTNODE__HH


#include "machine/disk.hh"

static const unsigned NUM_DIRECT
  = SECTOR_SIZE  / sizeof (int); // Esto se guarda en un sector (128 Bytes)
                                // por lo que calculamos cuantos bytes podemos 
                                // guardar en el mismo.

struct RawIndirectNode {
    unsigned dataSectors[NUM_DIRECT];  ///< Disk sector numbers for each data
                                       ///< block in the file.
};

#endif /*NACHOS_FILESYS_RAWINDIRECTNODE__HH*/
