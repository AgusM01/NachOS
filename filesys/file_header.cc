/// Routines for managing the disk file header (in UNIX, this would be called
/// the i-node).
///
/// The file header is used to locate where on disk the file's data is
/// stored.  We implement this as a fixed size table of pointers -- each

/// the file data (in other words, there are no indirect or doubly indirect
/// blocks). The table size is chosen so that the file header will be just
/// big enough to fit in one disk sector,
///
/// Unlike in a real system, we do not keep track of file permissions,
/// ownership, last modification date, etc., in the file header.
///
/// A file header can be initialized in two ways:
///
/// * for a new file, by modifying the in-memory data structure to point to
///   the newly allocated data blocks;
/// * for a file already on disk, by reading the file header from disk.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "file_header.hh"
#include "file_system.hh"
#include "lib/utility.hh"
#include "threads/system.hh"

#define MIN(a,b) a < b ? a : b
#define MAX(a,b) a > b ? a : b


#include <ctype.h>
#include <stdio.h>


/// Initialize a fresh file header for a newly created file.  Allocate data
/// blocks for the file out of the map of free disk blocks.  Return false if
/// there are not enough free blocks to accomodate the new file.
///
/// * `freeMap` is the bit map of free disk sectors.
/// * `fileSize` is the bit map of free disk sectors.
bool
FileHeader::Allocate(Bitmap *freeMap, unsigned fileSize)
{

    ASSERT(freeMap != nullptr);
    
    // Con doble indirección MAX_FILE_SIZE es DISK_SIZE.
    if (fileSize > MAX_FILE_SIZE) {
        return false;
    }

    raw.numBytes = fileSize;
    raw.numSectors = DivRoundUp(fileSize, SECTOR_SIZE);
    
    unsigned cantIndirects1 = DivRoundUp(raw.numSectors, NUM_DIRECT*NUM_DIRECT);
    unsigned cantIndirects2 = DivRoundUp(raw.numSectors, NUM_DIRECT);
    unsigned cantIndirectsTotal = cantIndirects1 + cantIndirects2;
    if (freeMap->CountClear() < raw.numSectors + cantIndirectsTotal) {
        return false;  // Not enough space.
    }
    
    DEBUG('f', "Hay espacio para %u sectores\n", raw.numSectors);
    DEBUG('f', "En el bitmap hay disponibles %u sectores\n", freeMap->CountClear());

    // Para ejercicio 2:
    // Ahora tengo NUM_INDIRECT punteros que pueden 
    // contener NUM_DIRECT punteros los cuales cada uno de ellos
    // contiene NUM_DIRECT sectores.
    

    unsigned sectorsLeft = raw.numSectors;
    
    for (unsigned i = 0; (i < NUM_INDIRECT && sectorsLeft > 0); i++)
    {
        ASSERT((int)(raw.dataSectors[i] = freeMap->Find()) != -1);
        for (unsigned j = 0; (j < NUM_DIRECT && sectorsLeft > 0); j++)
        {
            ASSERT((int)(raw_ind[i].dataSectors[j] = freeMap->Find()) != -1);
            for (unsigned k = 0; k < NUM_DIRECT && sectorsLeft > 0; k++)
            {
                ASSERT((int)(raw_ind2[i][j].dataSectors[k] = freeMap->Find()) != -1);
                sectorsLeft -= 1;
                DEBUG('f', "Sectores allocados: %u\n", raw.numSectors - sectorsLeft);
                DEBUG('f', "Espacio en bitmap: %u\n", freeMap->CountClear());
            }
        }
    }

    return true;
}

/// De-allocate all the space allocated for data blocks for this file.
///
/// * `freeMap` is the bit map of free disk sectors.
/// Su concurrencia viene dada por Remove de file_system.cc.
void
FileHeader::Deallocate(Bitmap *freeMap)
{
    // Elimina solo los datos. No el header.
    ASSERT(freeMap != nullptr);

    unsigned sectorsLeft = raw.numSectors;
    
    for (unsigned i = 0; (i < NUM_INDIRECT && sectorsLeft > 0); i++)
    {
        // Traigo el primer nivel de indirección del disco.
        synchDisk->ReadSector(raw.dataSectors[i], (char *) &raw_ind[i]);
        for (unsigned j = 0; (j < NUM_DIRECT && sectorsLeft > 0); j++)
        {
            // Traigo el segundo nivel de indirección del disco. 
            synchDisk->ReadSector(raw_ind[i].dataSectors[j], (char*) &raw_ind2[i][j]);
            for (unsigned k = 0; k < NUM_DIRECT && sectorsLeft > 0; k++)
            {
                ASSERT(freeMap->Test(raw_ind2[i][j].dataSectors[k]));
                freeMap->Clear(raw_ind2[i][j].dataSectors[k]);
                sectorsLeft -= 1;
            }
            ASSERT(freeMap->Test(raw_ind[i].dataSectors[j]));
            freeMap->Clear(raw_ind[i].dataSectors[j]);
        }
        ASSERT(freeMap->Test(raw.dataSectors[i]));
        freeMap->Clear(raw.dataSectors[i]);
    }
   return; 
}

/// Fetch contents of file header from disk.
///
/// * `sector` is the disk sector containing the file header.
///
void
FileHeader::FetchFrom(unsigned sector)
{
    
    DEBUG('f', "Traigo fileHeader del sector %u\n", sector);
    synchDisk->ReadSector(sector, (char *) &raw);
    
    DEBUG('f', "La cantidad de sectores son: %u\n", raw.numSectors); 
    unsigned cantIndirects1 = DivRoundUp(raw.numSectors, NUM_DIRECT*NUM_DIRECT);
    unsigned cantIndirects2 = DivRoundUp(raw.numSectors, NUM_DIRECT);

    for (unsigned i = 0; i < cantIndirects1; i++){
        synchDisk->ReadSector(raw.dataSectors[i], (char *) &raw_ind[i]);
        for (unsigned j = 0; j < cantIndirects2; j++){
            synchDisk->ReadSector(raw_ind[i].dataSectors[j], (char *) &raw_ind2[i][j]);
        }
    }

    DEBUG('f', "Termino de traer el fileHeader\n");
    return;
}

/// Write the modified contents of the file header back to disk.
///
/// * `sector` is the disk sector to contain the file header.
///
void
FileHeader::WriteBack(unsigned sector)
{
    DEBUG('f', "Escribo en el disco el sector %u\n", sector);

    unsigned cantIndirects1 = DivRoundUp(raw.numSectors, NUM_DIRECT*NUM_DIRECT);
    unsigned cantIndirects2 = DivRoundUp(raw.numSectors, NUM_DIRECT);
    
    synchDisk->WriteSector(sector, (char *) &raw);
    
    for (unsigned i = 0; i < cantIndirects1; i++){
        synchDisk->WriteSector(raw.dataSectors[i], (char *) &raw_ind[i]);
        for (unsigned j = 0; j < cantIndirects2; j++){
            synchDisk->WriteSector(raw_ind[i].dataSectors[j], (char*) &raw_ind2[i][j]);
        }
    }
    
    return;
}

/// Return which disk sector is storing a particular byte within the file.
/// This is essentially a translation from a virtual address (the offset in
/// the file) to a physical address (the sector where the data at the offset
/// is stored).
///
/// * `offset` is the location within the file of the byte in question.
unsigned
FileHeader::ByteToSector(unsigned offset)
{
    DEBUG('f', "Trayendo el byte %u\n", offset);
    unsigned numDirect = offset / SECTOR_SIZE;
    
    DEBUG('f', "El byte está en el directo %u\n", numDirect);

    unsigned cantIndirects1 = DivRoundUp(raw.numSectors, NUM_DIRECT*NUM_DIRECT);
    unsigned cantIndirects2 = DivRoundUp(raw.numSectors, NUM_DIRECT);
    
    unsigned indirecLevel1 = cantIndirects1 == 0 ? 0 : numDirect % cantIndirects1;
    unsigned indirecLevel2 = cantIndirects2 == 0 ? 0 : numDirect % cantIndirects2;
    
    DEBUG('f', "El primer nivel de indirección es %u\n", indirecLevel1);
    DEBUG('f', "El segundo nivel de indirección es %u\n", indirecLevel2);

    unsigned direcInLevel = numDirect - (NUM_DIRECT*NUM_DIRECT * indirecLevel1 + NUM_DIRECT*indirecLevel2);
    
    DEBUG('f', "El numero de dirección buscado es el numero %u dentro de su indirección\n", direcInLevel);

    RawIndirectNode rin_1;
    RawIndirectNode rin_2;

    synchDisk->ReadSector(raw.dataSectors[indirecLevel1], (char *) &rin_1);
    synchDisk->ReadSector(rin_1.dataSectors[indirecLevel2], (char *) &rin_2);
    
    DEBUG('f', "ByteToSector el sector que devuelvo es el: %u\n", rin_2.dataSectors[direcInLevel]);

    return rin_2.dataSectors[direcInLevel];
}

bool
FileHeader::AddSectors(unsigned sector, unsigned lastSector, unsigned addBytes)
{
    Bitmap *freeMap = new Bitmap(NUM_SECTORS);
    freeMap->FetchFrom(fileTable->GetFile("freeMap"));

    // Primero traigo el fileHeader.
    FetchFrom(sector);
    
    DEBUG('f', "Voy a agregar sectores\n");

    if (raw.numBytes + addBytes > MAX_FILE_SIZE){
        DEBUG('f', "No es posible agregar más contenido al archivo.\n");
        return false;
    }
    
    // Calculo cuántos sectores tengo que agregar.
    ASSERT(lastSector >= raw.numSectors);
    unsigned newSectors = raw.numBytes != 0 ? lastSector - raw.numSectors : 1;
    
    DEBUG('f', "El arhivo tiene %u bytes y debo agregar %u bytes, por eso debo agregar %u sectores\n", raw.numBytes, addBytes, newSectors);

    if (newSectors == 0){
        DEBUG('f', "No es necesario agregar sectores\n");
        raw.numBytes += addBytes;
        return false;
    }

    if (raw.numSectors + newSectors > freeMap->CountClear()){
        DEBUG('f', "No es posible agregar más sectores a este archivo.\n");
        return false;
    }

    // Tengo que calcular cuál es el último direct utilizado para agregar sectores
    // a partir de ahí.
    // Si bien la información está toda separada en el disco, dentro del fileHeader 
    // sigue un orden y gracias a esto podemos decir que el primer direct va a contener
    // los primeros bytes y el último, los últimos.
    
    unsigned numDirect = raw.numSectors;

    unsigned cantIndirects1 = DivRoundUp(raw.numSectors, NUM_DIRECT*NUM_DIRECT);
    unsigned cantIndirects2 = DivRoundUp(raw.numSectors, NUM_DIRECT);

    unsigned indirecLevel1 = cantIndirects1 == 0 ? 0 : numDirect % cantIndirects1;
    unsigned indirecLevel2 = cantIndirects2 == 0 ? 0 : numDirect % cantIndirects2;
    
    unsigned direcInLevel = numDirect - (NUM_DIRECT*NUM_DIRECT * indirecLevel1 + NUM_DIRECT*indirecLevel2);
    
    DEBUG('f', "El nivel de indirección 1 del último sector es %u.\n El nivel de indirección 2 del último sector es %u.\n El directo es el número %u, por lo que empiezo a agregar sectores en el %u.\n", indirecLevel1, indirecLevel2, direcInLevel, direcInLevel + 1);

    if (raw.numBytes != 0)
        direcInLevel += 1;

    unsigned newCantIndirects1 = DivRoundUp(raw.numSectors + newSectors, NUM_DIRECT*NUM_DIRECT);
    unsigned newCantIndirects2 = DivRoundUp(raw.numSectors + newSectors, NUM_DIRECT);
    unsigned leftSectors = newSectors;
    
    DEBUG('f', "La nueva primera indirección es %u.\n La nueva segunda indirección es %u\n", newCantIndirects1, newCantIndirects2); 

    for (unsigned i = indirecLevel1; i < newCantIndirects1 && leftSectors > 0; i++){
        if (i > indirecLevel1 || (raw.numBytes == 0 && i == indirecLevel1)){
            ASSERT((int)(raw.dataSectors[i] = freeMap->Find()) != -1);
            DEBUG('f', "Agrego el sector %u en el primer nivel de indirección %u\n", raw.dataSectors[i], i);
        }
        for (unsigned j = indirecLevel2; j < newCantIndirects2 && leftSectors > 0; j++){
            if (j > indirecLevel2 || (raw.numBytes == 0 && j == indirecLevel2)){
                ASSERT((int)(raw_ind[i].dataSectors[j] = freeMap->Find()) != -1);
                DEBUG('f', "Agrego el sector %u en el segundo nivel de indirección %u\n", raw_ind[i].dataSectors[j], j);
            }
            for (unsigned k = direcInLevel; k < NUM_DIRECT && leftSectors > 0; k++, leftSectors--){
                ASSERT((int)(raw_ind2[i][j].dataSectors[k] =  freeMap->Find()) != -1);
                DEBUG('f', "Agrego el sector %u en el 1er nivel de indirección %u, 2do nivel de indirección %u y nivel directo %u\n", raw_ind2[i][j].dataSectors[k], i, j, k);
            }
            direcInLevel = 0;
        }
        indirecLevel2 = 0;
    }

    raw.numSectors += newSectors;
    raw.numBytes += addBytes;

    // Mando todo a disco.
    WriteBack(sector);
    freeMap->WriteBack(fileTable->GetFile("freeMap"));
    delete freeMap;

    DEBUG('f', "Añadí %u sectores correctamente\n", newSectors);
    return true;
}

/// Return the number of bytes in the file.
unsigned
FileHeader::FileLength() const
{
    return raw.numBytes;
}

/// Print the contents of the file header, and the contents of all the data
/// blocks pointed to by the file header.
void
FileHeader::Print(const char *title)
{
    char *data = new char[SECTOR_SIZE];

    if (title == nullptr) {
        printf("File header:\n");
    } else {
        printf("%s file header:\n", title);
    }

    printf("    size: %u bytes\n"
           "    block indexes: ",
           raw.numBytes);

    unsigned cantIndirects1 = DivRoundUp(raw.numSectors, NUM_DIRECT*NUM_DIRECT);
    unsigned cantIndirects2 = DivRoundUp(raw.numSectors, NUM_DIRECT);

    unsigned located = 0;
    unsigned sectorsLeft = raw.numSectors;
    
    for (unsigned i = 0, k = 0; i < cantIndirects1; i++){
        synchDisk->ReadSector(raw.dataSectors[i], (char *) &raw_ind[i]);
        for (unsigned j = 0; j < cantIndirects2; j++){
            synchDisk->ReadSector(raw_ind[i].dataSectors[j], (char *) &raw_ind2[i][j]);
            located = MIN(NUM_DIRECT, sectorsLeft);
            for (unsigned z = 0; z < located; z++){
                printf("Contents of block %u:\n", raw_ind2[i][j].dataSectors[z]);
                synchDisk->ReadSector(raw_ind2[i][j].dataSectors[z], data);
                for (unsigned n = 0; n < SECTOR_SIZE && k < raw.numBytes; n++, k++){
                    
                    if (isprint(data[n])) {
                        printf("%c", data[n]);
                    } else {
                        printf("\\%X", (unsigned char) data[n]);
                    }
                }
                printf("\n");
            }
            sectorsLeft -= located;
            located = 0;
        } 
    }

    delete [] data;
    return;
}

const RawFileHeader *
FileHeader::GetRaw() const
{
    return &raw;
}
