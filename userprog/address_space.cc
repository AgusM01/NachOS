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
   
   // printf("numPages: %d\n", numPages);
    // Si hay DL no hacemos nada de esto.
    #ifdef DEMAND_LOADING
        return;
    #endif 
    char *mainMemory = machine->mainMemory; /// mainMemory es un arreglo de bytes.

    // Seteo en 0 los marcos que le pertenecen al proceso.
    DEBUG('a', "Seteando en 0 la memoria de las páginas.\n");
    for (unsigned i = 0; i < numPages; i++)
        memset(&mainMemory[PHYSICAL_PAGE_ADDR(i)], 0, PAGE_SIZE);

    // Then, copy in the code and data segments into memory.
    uint32_t codeSize = exe->GetCodeSize();
    //printf("codeSize: %d\n", codeSize);
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
    //printf("initDataSize: %d\n", initDataSize);
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
void
AddressSpace::LoadPage(unsigned badPageNumber)
{

    DEBUG('a',"Cargando página - DEMAND LOADING\n");

    char *mainMemory = machine->mainMemory;

    // Primero calculo que número de pagina falló.
    uint32_t badVAddr = badPageNumber * PAGE_SIZE;
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

   // puts("----------------------------------------------------------");
   // printf("codeAddr: %d\n", codeAddr);
   // printf("codeSize: %d\n", codeSize);
   // printf("endCodeAddr: %d\n", endCodeAddr);
   // printf("dataAddr: %d\n", dataAddr);
   // printf("dataSize: %d\n", dataSize);
   // printf("endDataAddr: %d\n", endDataAddr);
   // printf("badPageNumber: %d\n", badPageNumber);
   // printf("badVAddr: %d\n", badVAddr);
   // printf("botBadPage: %d\n", botBadPage);
   // printf("topBadPage: %d\n", topBadPage);
   // printf("offsetPage: %d\n", offsetPage);
   // puts("----------------------------------------------------------");
    
    // Es una dirección del stack. Relleno con 0 y listo.
    if (dataSize == 0){
        if (badVAddr > endCodeAddr)
        {
            pageTable[badPageNumber].valid = true;
            pageTable[badPageNumber].use = true;
            return;
        }
    }

    if (badVAddr > endDataAddr)
    {
        pageTable[badPageNumber].valid = true;
        pageTable[badPageNumber].use = true;
        return;
    }
    
    //Aca tengo 3 casos:
    //  *) Toda la página es de código.
    //  *) Toda la página es de datos.
    //  *) Hay un poco y un poco.

    int fileOffset;
    

    // Primer caso, toda la página es de código
    if (codeSize > 0 && codeAddr <= botBadPage && (dataSize == 0 || topBadPage <= dataAddr))
    {
        fileOffset = badVAddr - codeAddr;
        //puts("Toda la página es de código");
        exe->ReadCodeBlock(
          &mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber)],
          PAGE_SIZE,
          fileOffset
        );
        pageTable[badPageNumber].valid = true;
        pageTable[badPageNumber].use = true;
        return;
    }

    // Segundo caso, toda la página es de datos.
    if (dataSize > 0 && dataAddr <= botBadPage)
    {
        fileOffset = badVAddr - dataAddr;
        //puts("Toda la página es de datos");
        exe->ReadDataBlock(
          &mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber)],
          PAGE_SIZE,
          fileOffset
        );
        pageTable[badPageNumber].valid = true;
        pageTable[badPageNumber].use = true;
        return;
    }

    // Tercer caso, un poco y un poco.
    if (codeSize > 0 && dataSize > 0 && codeAddr <= botBadPage && botBadPage < dataAddr)
    {
       // puts("Hay un poco y un poco");
        int writeCode = endCodeAddr - botBadPage;
        int writeData = PAGE_SIZE - writeCode;
        
        fileOffset = badVAddr - codeAddr;
        exe->ReadCodeBlock(
          &mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber)],
          writeCode,
          fileOffset
        );
        
        fileOffset = 0;
        exe->ReadDataBlock(
          &mainMemory[PHYSICAL_PAGE_ADDR(badPageNumber) + writeCode],
          writeData,
          fileOffset
        );
        
        pageTable[badPageNumber].valid = true;
        pageTable[badPageNumber].use = true;
        
        return;

    }
    puts("No cargo nada");
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
        //puts("Entro a DEMANDLOADING");    
        //printf("ANTES: pageTable[badPageNumber].valid: %d\n", pageTable[badPageNumber].valid);
        // Busco un lugar en la memoria libre.
        pageTable[badPageNumber].physicalPage = bit_map->Find();
        ASSERT((int)pageTable[badPageNumber].physicalPage != -1);
        LoadPage(badPageNumber);
        //printf("DESPUES: pageTable[badPageNumber].valid: %d\n", pageTable[badPageNumber].valid);
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
