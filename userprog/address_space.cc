/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include <cstdint>
#define MIN(a,b) a < b ? a : b
#define MAX(a,b) a > b ? a : b
#define PHYSICAL_PAGE_ADDR(virtualPage) pageTable[virtualPage].physicalPage * PAGE_SIZE 


#include "address_space.hh"
#include "executable.hh"
#include "lib/utility.hh"
#include "threads/system.hh"

#include <stdio.h>
#include <string.h>

/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *executable_file)
{
    ASSERT(executable_file != nullptr);

    /// Creo el ejecutable
    #ifndef DEMAND_LOADING
    Executable *exe = new Executable(executable_file);
    #else
    exe_file = executable_file;
    exe = new Executable(executable_file);
    #endif

    ASSERT(exe->CheckMagic());

    /// Lo cargo en memoria
    // How big is address space?
    unsigned size = exe->GetSize() + USER_STACK_SIZE; /// Lo que ocupa el .text + el stack.
   
        // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE; /// Recalcula el nuevo tamaño con las páginas de mas incluidas.
    
    // Al tener DL no se va a cargar todo el programa en memoria.
    // Se irá cargando de a partes y quizás se va liberando la memoria
    // Además quizás hay secciones del código que no se cargarán nunca y el programa entraria igual.
    
    //#ifndef DEMAND_LOADING
    ASSERT(numPages <= bit_map->CountClear()); /// Calculamos la cantidad de espacio disponible según el bitmap      // Check we are not trying to run anything too big -- at least until we
      // have virtual memory.
    //#endif

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.
    
    // Al momento de usar páginas, marcarlas como usada. Cuando se terminan de usar, marcarlas como libre.
    pageTable = new TranslationEntry[numPages]; /// Crea la tabla de paginacion
    for (unsigned i = 0; i < numPages; i++) { /// Asigna 1:1 las páginas con la memoria fisica. 
        pageTable[i].virtualPage  = i;
        // Si estamos usando DL no es necesario buscar los marcos.
        // Los vamos cargando a medida que el programa va pidiendo las secciones de memoria.
        #ifdef DEMAND_LOADING
        pageTable[i].physicalPage = -1;
        pageTable[i].valid        = false;
        #else 
        /// Devolvemos el primer lugar de la memoria física libre. 
        pageTable[i].physicalPage = bit_map->Find();
        pageTable[i].valid = true;
        #endif
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
          // If the code segment was entirely on a separate page, we could
          // set its pages to be read-only.
    }
    
    // Si hay DL no hacemos nada de esto.
    #ifdef DEMAND_LOADING
        dlOffsetFile = 0;
        return;
    #endif 
    char *mainMemory = machine->mainMemory; /// mainMemory es un arreglo de bytes.

    //printf("MEMORY: %p\n", &machine->mainMemory);

    // Seteo en 0 los marcos que le pertenecen al proceso.
    DEBUG('a', "Seteando en 0 la memoria de las páginas.\n");
    for (unsigned i = 0; i < numPages; i++)
        memset(&mainMemory[PHYSICAL_PAGE_ADDR(i)], 0, PAGE_SIZE);

    // Then, copy in the code and data segments into memory.
    uint32_t codeSize = exe->GetCodeSize();
    if (codeSize > 0) {
        // virtualAddr es la direccion generada por el proceso.
        // Por arquitectura MIPSEL, siempre es 0.
        uint32_t virtualAddr = exe->GetCodeAddr();
        DEBUG('a', "Initializing code segment, at 0x%X, size %u\n",
              virtualAddr, codeSize);
        uint32_t offsetPage = virtualAddr % PAGE_SIZE; // Desplazamiento dentro de la página.
        uint32_t offsetFile = 0; // Estoy al inicio del archivo.
        unsigned virtualPage = virtualAddr / PAGE_SIZE; // Da el numero de página. Es división entera. 
        uint32_t firstPageWriteSize = MIN(PAGE_SIZE - offsetPage, codeSize);
        uint32_t remainingToWrite = codeSize - firstPageWriteSize;


        DEBUG('a', "Write at 0x%X, size %u\n",
              PHYSICAL_PAGE_ADDR(virtualPage) + offsetPage, firstPageWriteSize);

        
        //printf("MEMORY: %p\n", &mainMemory[PHYSICAL_PAGE_ADDR(virtualPage) + offsetPage]);
        //printf("VIRTUALPAGE: %d\n", virtualPage);
        //printf("OFFSETPAGE: %d\n", offsetPage);
        //printf("A DONDE EN MEMORY: %d\n", PHYSICAL_PAGE_ADDR(virtualPage) + offsetPage);
        //printf("MEMORY: %p\n", mainMemory);
        exe->ReadCodeBlock(
            &mainMemory[PHYSICAL_PAGE_ADDR(virtualPage) + offsetPage],
            firstPageWriteSize,
            offsetFile
        );
        pageTable[virtualPage].use = true;

        if (remainingToWrite > 0) {
            DEBUG('a', "Left to write: %u.\n", remainingToWrite);

            offsetFile = firstPageWriteSize; // Desplazamiento en Disco
            virtualPage++;                   // Pasamos a la siguiente página virtual

            // Si lo que falta escribir supera el tamaño de una página,
            // tenemos que escribir una página entera
            while(PAGE_SIZE < remainingToWrite) 
            {
                DEBUG('a', "Write at 0x%X, size %u\n",
                    PHYSICAL_PAGE_ADDR(virtualPage), PAGE_SIZE);

                exe->ReadCodeBlock(&mainMemory[PHYSICAL_PAGE_ADDR(virtualPage)], PAGE_SIZE, offsetFile);
                pageTable[virtualPage].use = true;

                offsetFile += PAGE_SIZE; // Avanzamos en el Disco 
                remainingToWrite -= PAGE_SIZE; // Nos falta una página menos.

                // La página es solo del segmento del código, marcamos como solo lectura.
                pageTable[virtualPage].readOnly = true; 

                // Avanzamos a la página siguiente.
                // Sabes que es todo contiguo por ser memoria virtual.
                virtualPage++;
            }

                DEBUG('a', "Last Write at 0x%X, size %u\n",
                    PHYSICAL_PAGE_ADDR(virtualPage), remainingToWrite);

            // Ultima escritura, ya que remainingToWrite <= PAGE_SIZE
            exe->ReadCodeBlock(&mainMemory[PHYSICAL_PAGE_ADDR(virtualPage)], remainingToWrite, offsetFile);
            pageTable[virtualPage].use = true;

            //Si la útima escritura es una página entera, marcamos la página como solo lecutura.
            if (remainingToWrite == PAGE_SIZE)
                pageTable[virtualPage].readOnly = true;
        }

    }
    
    // Exactamente lo mismo pero con el data segment.
    uint32_t initDataSize = exe->GetInitDataSize();

    if (initDataSize > 0) {
        uint32_t virtualAddr = exe->GetInitDataAddr();
        DEBUG('a', "Initializing data segment, at 0x%X, size %u\n",
              virtualAddr, initDataSize);
        uint32_t offsetPage = virtualAddr % PAGE_SIZE;
        uint32_t offsetFile = 0;
        unsigned virtualPage = virtualAddr / PAGE_SIZE;
        uint32_t firstPageWriteSize = MIN(PAGE_SIZE - offsetPage, initDataSize);
        uint32_t remainingToWrite = initDataSize - firstPageWriteSize;

        DEBUG('a', "Write at 0x%X, size %u, offsetPage %u, remainingToWrite %u\n",
              PHYSICAL_PAGE_ADDR(virtualPage) + offsetPage, firstPageWriteSize, offsetPage, remainingToWrite);
        
        exe->ReadDataBlock(&mainMemory[PHYSICAL_PAGE_ADDR(virtualPage) + offsetPage], firstPageWriteSize, offsetFile);
        pageTable[virtualPage].use = true;
        if (remainingToWrite > 0) {
            offsetFile = firstPageWriteSize; // Desplazamiento en Disco
            virtualPage++;                   // Pasamos a la siguiente página virtual

            // Si lo que falta escribir supera el tamaño de una página,
            // tenemos que escribir una página entera
            while(PAGE_SIZE < remainingToWrite) 
            {
                DEBUG('a', "Write in while at 0x%X, size %u, remainingToWrite\n",
                    PHYSICAL_PAGE_ADDR(virtualPage) + offsetPage, PAGE_SIZE, remainingToWrite);

                exe->ReadDataBlock(&mainMemory[PHYSICAL_PAGE_ADDR(virtualPage)], PAGE_SIZE, offsetFile);
                pageTable[virtualPage].use = true;

                offsetFile += PAGE_SIZE; // Avanzamos en el Disco 
                remainingToWrite -= PAGE_SIZE; // Nos falta una página menos.

                // Avanzamos a la página siguiente
                virtualPage++;
            }

                DEBUG('a', "Write at last 0x%X, size %u\n",
                    PHYSICAL_PAGE_ADDR(virtualPage) + offsetPage, remainingToWrite);

            // Ultima escritura, ya que remainingToWrite <= PAGE_SIZE
            exe->ReadDataBlock(&mainMemory[PHYSICAL_PAGE_ADDR(virtualPage)], remainingToWrite, offsetFile);
            pageTable[virtualPage].use = true;
        }
    }
}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
    //printf("Elimino pageTable, soy %s\n", currentThread->GetName());
    for (unsigned i = 0; i < numPages; i++)
        if (((int)pageTable[i].physicalPage) != -1)
            bit_map->Clear(pageTable[i].physicalPage);
    
    // Ver.    
    //#ifdef DEMAND_LOADING
    //    delete exe_file;
    //#endif
    delete [] pageTable;
}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void
AddressSpace::InitRegisters()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++) {
        machine->WriteRegister(i, 0);
    }

    // Initial program counter -- must be location of `Start`.
    machine->WriteRegister(PC_REG, 0);

    // Need to also tell MIPS where next instruction is, because of branch
    // delay possibility.
    machine->WriteRegister(NEXT_PC_REG, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we do not
    // accidentally reference off the end!
    machine->WriteRegister(STACK_REG, numPages * PAGE_SIZE - 16);
    DEBUG('a', "Initializing stack register to %u\n",
          numPages * PAGE_SIZE - 16);
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void
AddressSpace::SaveState()
{
    //printf("Guardo estado, soy %s\n", currentThread->GetName());
    #ifdef USE_TLB
    TranslationEntry* tlb = machine->GetMMU()->tlb;
    for (unsigned int i = 0; i < TLB_SIZE ; i++)
    {
        if(tlb[i].valid) 
        {
            currentThread->space->pageTable[tlb[i].virtualPage].use = tlb[i].use;
            currentThread->space->pageTable[tlb[i].virtualPage].dirty = tlb[i].dirty;
        }
    }
    
    #endif
}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void
AddressSpace::RestoreState()
{
    #ifdef USE_TLB
    //Invalidar la TLB
    for (int i = 0; i < TLB_SIZE; i++)
        machine->GetMMU()->tlb[i].valid = false;
    #else
    /// Comentar estas dos
    machine->GetMMU()->pageTable     = pageTable;
    machine->GetMMU()->pageTableSize = numPages;
    #endif
}

#ifdef DEMAND_LOADING
//void 
//AddressSpace::LoadPage(unsigned vpn)
//{
//char* mainMemory = machine->mainMemory;
//    DEBUG('y',"RetrievePage: Invalid Page: %u, start loading process.\n", vpn);
//
//    // Direccion virtual del incio de la página
//    unsigned pageDownAddress = vpn * PAGE_SIZE; 
//
//    // Direccion de la siguiente página (tope de la página actual)
//    unsigned pageUpAddress = pageDownAddress + PAGE_SIZE;
//
//    unsigned codeAddr = exe->GetCodeAddr();          //Inicio segmento código
//    unsigned codeSize = exe->GetCodeSize();          //Tamaño del segmento de código
//    unsigned dataAddr = exe->GetInitDataAddr();      //Inicio segmento datos
//    unsigned dataSize = exe->GetInitDataSize();      //Tamaño del segmento de datos
//
//    DEBUG('y', "Write vp  %u, initAddr %u, size %u\n",
//        vpn, PHYSICAL_PAGE_ADDR(vpn), PAGE_SIZE); 
//
//    memset(&mainMemory[PHYSICAL_PAGE_ADDR(vpn)], 0, PAGE_SIZE);
//
//    //Necesito escribir algo del segmento de código
//    if (codeSize > 0 && codeAddr <= pageDownAddress && pageDownAddress < codeAddr + codeSize){ 
//        //Parte de direcciones virtuales
//        unsigned firstAddre = MAX(pageDownAddress, codeAddr); // En nachos podríamos dejar solo pageDownAddress
//        ASSERT(firstAddre == pageDownAddress);
//        unsigned offSet = firstAddre % PAGE_SIZE;
//        ASSERT(offSet == 0);
//        unsigned bytesToWrite = MIN(codeAddr + codeSize - firstAddre, pageUpAddress - firstAddre);
//
//        //Si lo que escribimos en el segmento de código es un página entera,
//        //entonces la página se puede marcar como read only
//        if (bytesToWrite == PAGE_SIZE){
//            pageTable[vpn].readOnly = true;
//        }
//
//        //Parte el archivo del ejecutable
//        unsigned fileOffSet = firstAddre - codeAddr;
//
//        //Escritura en la página física
//        exe->ReadCodeBlock(&mainMemory[PHYSICAL_PAGE_ADDR(vpn) + offSet], bytesToWrite, fileOffSet);
//    }
//
//    //Necesito escribir algo del segmento de datos inicializados
//    if (dataSize > 0 && dataAddr < pageUpAddress && pageDownAddress < dataAddr + dataSize){
//        //Parte de direcciones virtuales
//        unsigned firstAddre = MAX(pageDownAddress, dataAddr); //Acá es necesario el max
//        unsigned offSet = firstAddre % PAGE_SIZE;
//        unsigned bytesToWrite = MIN(dataAddr + dataSize - firstAddre, pageUpAddress - firstAddre);
//
//        //Parte el archivo del ejecutable
//        unsigned fileOffSet = firstAddre - dataAddr;
//
//        //Escritura en la página física
//        exe->ReadDataBlock(&mainMemory[PHYSICAL_PAGE_ADDR(vpn) + offSet], bytesToWrite, fileOffSet);
//    }
//}
void
AddressSpace::LoadPage(unsigned badPageNumber)
{
    DEBUG('a',"Cargando página - DEMAND LOADING\n");

    char *mainMemory = machine->mainMemory;

    // Primero calculo que número de pagina falló.
    // Junto con su offset.
    uint32_t badVAddr = badPageNumber * PAGE_SIZE;
    uint32_t offsetPage = badVAddr % PAGE_SIZE;
    uint32_t  botBadPage = badPageNumber * PAGE_SIZE;
    uint32_t  topBadPage = botBadPage + PAGE_SIZE;

    // Seteo en 0 el marco a utilizar.
    memset(&mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber)], 0, PAGE_SIZE);

    // Son direcciones lógicas.
    uint32_t codeSize = exe->GetCodeSize();
    uint32_t codeAddr = exe->GetCodeAddr();
    uint32_t endCodeAddr  = (codeAddr + codeSize);
    uint32_t dataSize = exe->GetInitDataSize();
    uint32_t dataAddr = exe->GetInitDataAddr();
    uint32_t endDataAddr  = (dataAddr + dataSize);
    
    // Es una dirección del stack. Relleno con 0 y listo.
    if (badVAddr > codeSize + dataSize)
        return;

    //Aca tengo 3 casos:
    //  *) Estoy al comienzo de todo.
    //  *) Estoy en el medio.
    //  *) Estoy al final.

    // Lo que voy a buscar escribir.
    int toWrite;

    // *) Estoy al comienzo.
    if (badPageNumber == 0)
    {
        puts("-----------------------------------------------------------------------------");
        puts("Estoy al comienzo.");
        toWrite = PAGE_SIZE - offsetPage;

        // En este caso el código ocupa toda la página.
        if (codeSize > 0 && codeAddr <= topBadPage && topBadPage <= endCodeAddr)
        {
            puts("En este caso el código ocupa toda la página.");
            exe->ReadCodeBlock(
                &mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber) + offsetPage],
                toWrite,
                dlOffsetFile
            );
            dlOffsetFile += toWrite;
            pageTable[badPageNumber].valid = true;
            pageTable[badPageNumber].use = true;
            return;
        }

        // En este caso la cantidad de código no llega a ocupar enteramente la primer página.
        if (codeSize > 0 && codeAddr <= topBadPage && topBadPage > endCodeAddr)
        {
            puts("En este caso la cantidad de código no llega a ocupar enteramente la primer página.");
            exe->ReadCodeBlock(
                &mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber) + offsetPage],
                codeSize,
                dlOffsetFile
            );
            toWrite -= codeSize;
            dlOffsetFile += codeSize;
            offsetPage += codeSize;

            // Cargamos el segmento de datos.
            if (dataSize > 0 && dataAddr <= topBadPage && topBadPage <= endDataAddr)
            {
                puts("Cargamos el segmento de datos.");
                exe->ReadDataBlock(
                &mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber) + offsetPage],
                toWrite,
                dlOffsetFile
                );

                dlOffsetFile += toWrite;
                pageTable[badPageNumber].valid = true;
                pageTable[badPageNumber].use = true;
                return;
            }

            // El segmento de datos no llega a ocupar tampoco una página.
            if (dataSize > 0 && dataAddr <= topBadPage && topBadPage > endDataAddr)
            {
                puts("El segmento de datos no llega a ocupar tampoco una página.");
                exe->ReadDataBlock(
                    &mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber) + offsetPage],
                    dataSize,
                    dlOffsetFile
                );
                dlOffsetFile += dataSize;
            }
            
            pageTable[badPageNumber].valid = true;
            pageTable[badPageNumber].use = true;
            return;
        }

        // No tenemos código.
        // Cargamos el segmento de datos.
        if (dataSize > 0 && dataAddr <= topBadPage && topBadPage <= endDataAddr)
        {
            puts("No tenemos código, cargamos el segmento de datos");
            exe->ReadDataBlock(
            &mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber) + offsetPage],
            toWrite,
            dlOffsetFile
            );

            dlOffsetFile += toWrite;
            pageTable[badPageNumber].valid = true;
            pageTable[badPageNumber].use = true;
            return;
        }

        // El segmento de datos no llega a ocupar tampoco una página.
        // Este seria el caso en que haya una sola página.
        if (dataSize > 0 && dataAddr <= topBadPage && topBadPage > endDataAddr)
        {
            puts("El segmento de datos no llega a ocupar tampoco una página. Este seria el caso en que haya solo una página.");
            exe->ReadDataBlock(
                &mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber) + offsetPage],
                dataSize,
                dlOffsetFile
            );
            dlOffsetFile += dataSize;
        }

        pageTable[badPageNumber].valid = true;
        pageTable[badPageNumber].use = true;
        return;
    }

    // *) Estoy en el medio o al final.
    puts("----------------------------------------------------------------------");
    puts("Estoy en el medio o al final");
    // Mejor caso posible, toda la página es de código.
    if (codeSize > 0 && codeAddr <= botBadPage && topBadPage <= endCodeAddr)
    {
        puts("Mejor caso posible, toda la página es de código.");
        exe->ReadCodeBlock(
            &mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber)],
            PAGE_SIZE,
            dlOffsetFile
        );
        dlOffsetFile += PAGE_SIZE;
        pageTable[badPageNumber].valid = true;
        pageTable[badPageNumber].use = true;
        return;
    }

    // No toda la página es de código.
    if (codeSize > 0 && codeAddr <= botBadPage && topBadPage > endCodeAddr)
    {
        puts("No toda la página es de código.");

        // Cargo lo que hay de código.
        puts("Cargo lo que hay de código.");
        exe->ReadCodeBlock(
            &mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber)],
            endCodeAddr - botBadPage,
            dlOffsetFile
        );
        dlOffsetFile += (endCodeAddr - botBadPage);
        offsetPage = (endCodeAddr - botBadPage);

        // Si el resto de la página es datos.
        if (dataSize > 0 && dataAddr <= topBadPage && topBadPage <= endDataAddr)
        {
            puts("Si el resto de la página es datos.");
            toWrite = PAGE_SIZE - (endCodeAddr - botBadPage);
            exe->ReadDataBlock(
                &mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber) + offsetPage],
                toWrite,
                dlOffsetFile
            );
            dlOffsetFile += toWrite;
            pageTable[badPageNumber].valid = true;
            pageTable[badPageNumber].use = true;
            return;
        }

        // Si es la ultima página y los datos ocupan menos.
        // dataSize porque ya cargué código, entonces el segmento 
        // de datos esta contenido enteramente en esta página.
        if (dataSize > 0 && dataAddr <= topBadPage && topBadPage > endDataAddr)
        {
            puts("Si es la ultima pagina y los datos ocupan menos. Segmento de datos enteramente contenido en esta página.");
            exe->ReadDataBlock(
                &mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber) + offsetPage],
                dataSize,
                dlOffsetFile
            );
            dlOffsetFile += dataSize;
        }
        pageTable[badPageNumber].valid = true;
        pageTable[badPageNumber].use = true;
        return;
    }

    // No tenemos código.
    // Cargamos el segmento de datos.

    puts("No tenemos código, cargamos el segmento de datos.");

    // El segmento de datos ocupa toda la página.
    if (dataSize > 0 && dataAddr <= botBadPage && topBadPage <= endDataAddr)
    {
        puts("El segmento de datos ocupa toda la página.");
        exe->ReadDataBlock(
        &mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber)],
        PAGE_SIZE,
        dlOffsetFile
        );

        dlOffsetFile += PAGE_SIZE;
        pageTable[badPageNumber].valid = true;
        pageTable[badPageNumber].use = true;
        return;
    }

    // Estariamos en la última página.
    if (dataSize > 0 && dataAddr <= botBadPage && topBadPage > endDataAddr)
    {
        puts("Estaríamos en la última página.");
        exe->ReadDataBlock(
            &mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber)],
            endDataAddr - botBadPage,
            dlOffsetFile
        );
        dlOffsetFile += endDataAddr - botBadPage;
        pageTable[badPageNumber].valid = true;
        pageTable[badPageNumber].use = true;
        return;
    }

    // Esta condición no se puede dar.
    // No habría programa a cargar.
    ASSERT(false);
}
#endif

void 
AddressSpace::UpdateTLB(unsigned indexTlb, unsigned badVAddr) 
{
    
    unsigned badPageNumber = badVAddr / PAGE_SIZE;
    
    // Es el primer acceso y hay que cargar la página, esto es por DL.
    #ifdef DEMAND_LOADING
    if ((int)pageTable[badPageNumber].physicalPage == -1){
        
        // Busco un lugar en la memoria libre.
        pageTable[badPageNumber].physicalPage = bit_map->Find();
        ASSERT((int)pageTable[badPageNumber].physicalPage != -1);
        
        LoadPage(badPageNumber);
    }
    #endif

    MMU* MMU = machine->GetMMU(); 
    
    // Si es válida tengo que actualizar la pageTable.
    if (MMU->tlb[indexTlb].valid) 
    {
        unsigned pageNumber = MMU->tlb[indexTlb].virtualPage;
        bool valid = MMU->tlb[indexTlb].valid;
        bool use = MMU->tlb[indexTlb].use;
        bool dirty = MMU->tlb[indexTlb].dirty;
        bool readOnly = MMU->tlb[indexTlb].readOnly;    
        pageTable[pageNumber].valid = valid;
        pageTable[pageNumber].use = use;
        pageTable[pageNumber].dirty = dirty;
        pageTable[pageNumber].readOnly = readOnly;
    }

    MMU->tlb[indexTlb].virtualPage = pageTable[badPageNumber].virtualPage;
    MMU->tlb[indexTlb].physicalPage = pageTable[badPageNumber].physicalPage;
    MMU->tlb[indexTlb].valid = pageTable[badPageNumber].valid;
    MMU->tlb[indexTlb].use = pageTable[badPageNumber].use;
    MMU->tlb[indexTlb].dirty = pageTable[badPageNumber].dirty;
    MMU->tlb[indexTlb].readOnly = pageTable[badPageNumber].readOnly;
}
