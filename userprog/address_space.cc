/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include <cstdint>
#define min(a,b) a < b ? a : b
#define PYSHICAL_ADDR(virtualPage) pageTable[virtualPage].physicalPage * PAGE_SIZE 


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
    Executable exe (executable_file);
    ASSERT(exe.CheckMagic());
    
    /// Lo cargo en memoria
    // How big is address space?

    unsigned size = exe.GetSize() + USER_STACK_SIZE; /// Lo que ocupa el .text + el stack.
      // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE; /// Recalcula el nuevo tamaño con las páginas de mas incluidas.

    ASSERT(numPages <= bit_map->CountClear()); /// Calculamos la cantidad de espacio disponible según el bitmap      // Check we are not trying to run anything too big -- at least until we
      // have virtual memory.

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.
    
    // Al momento de usar páginas, marcarlas como usada. Cuando se terminan de usar, marcarlas como libre.
    pageTable = new TranslationEntry[numPages]; /// Crea la tabla de paginacion
    for (unsigned i = 0; i < numPages; i++) { /// Asigna 1:1 las páginas con la memoria fisica. 
        pageTable[i].virtualPage  = i;
          /// Devolvemos el primer lugar de la memoria física libre. 
        pageTable[i].physicalPage = bit_map->Find();
        pageTable[i].valid        = true;
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
          // If the code segment was entirely on a separate page, we could
          // set its pages to be read-only.
    }

    char *mainMemory = machine->mainMemory; /// mainMemory es un arreglo de bytes.

    // Seteo en 0 los marcos que le pertenecen al proceso.
    DEBUG('a', "Seteando en 0 la memoria de las páginas.\n");
    for (unsigned i = 0; i < numPages; i++)
        memset(&mainMemory[PYSHICAL_ADDR(i)], 0, PAGE_SIZE);

    // Then, copy in the code and data segments into memory.
    uint32_t codeSize = exe.GetCodeSize();
    if (codeSize > 0) {
        // virtualAddr es la direccion generada por el proceso.
        // Por arquitectura MIPSEL, siempre es 0.
        uint32_t virtualAddr = exe.GetCodeAddr();
        DEBUG('a', "Initializing code segment, at 0x%X, size %u\n",
              virtualAddr, codeSize);
        uint32_t offsetPage = virtualAddr % PAGE_SIZE; // Desplazamiento dentro de la página.
        uint32_t offsetFile = 0; // Estoy al inicio del archivo.
        unsigned virtualPage = virtualAddr / PAGE_SIZE; // Da el numero de página. Es división entera. 
        uint32_t firstPageWriteSize = min(PAGE_SIZE - offsetPage, codeSize);
        uint32_t remainingToWrite = codeSize - firstPageWriteSize;


        DEBUG('a', "Write at 0x%X, size %u\n",
              PYSHICAL_ADDR(virtualPage) + offsetPage, firstPageWriteSize);


        exe.ReadCodeBlock(
            &mainMemory[PYSHICAL_ADDR(virtualPage) + offsetPage],
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
                    PYSHICAL_ADDR(virtualPage), PAGE_SIZE);

                exe.ReadCodeBlock(&mainMemory[PYSHICAL_ADDR(virtualPage)], PAGE_SIZE, offsetFile);
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
                    PYSHICAL_ADDR(virtualPage), remainingToWrite);

            // Ultima escritura, ya que remainingToWrite <= PAGE_SIZE
            exe.ReadCodeBlock(&mainMemory[PYSHICAL_ADDR(virtualPage)], remainingToWrite, offsetFile);
            pageTable[virtualPage].use = true;

            //Si la útima escritura es una página entera, marcamos la página como solo lecutura.
            if (remainingToWrite == PAGE_SIZE)
                pageTable[virtualPage].readOnly = true;
        }

    }
    
    // Exactamente lo mismo pero con el data segment.
    uint32_t initDataSize = exe.GetInitDataSize();

    if (initDataSize > 0) {
        uint32_t virtualAddr = exe.GetInitDataAddr();
        DEBUG('a', "Initializing data segment, at 0x%X, size %u\n",
              virtualAddr, initDataSize);
        uint32_t offsetPage = virtualAddr % PAGE_SIZE;
        uint32_t offsetFile = 0;
        unsigned virtualPage = virtualAddr / PAGE_SIZE;
        uint32_t firstPageWriteSize = min(PAGE_SIZE - offsetPage, initDataSize);
        uint32_t remainingToWrite = initDataSize - firstPageWriteSize;

        DEBUG('a', "Write at 0x%X, size %u, offsetPage %u, remainingToWrite %u\n",
              PYSHICAL_ADDR(virtualPage) + offsetPage, firstPageWriteSize, offsetPage, remainingToWrite);

        exe.ReadDataBlock(&mainMemory[PYSHICAL_ADDR(virtualPage) + offsetPage], firstPageWriteSize, offsetFile);
        pageTable[virtualPage].use = true;
        if (remainingToWrite > 0) {
            offsetFile = firstPageWriteSize; // Desplazamiento en Disco
            virtualPage++;                   // Pasamos a la siguiente página virtual

            // Si lo que falta escribir supera el tamaño de una página,
            // tenemos que escribir una página entera
            while(PAGE_SIZE < remainingToWrite) 
            {
                DEBUG('a', "Write in while at 0x%X, size %u, remainingToWrite\n",
                    PYSHICAL_ADDR(virtualPage) + offsetPage, PAGE_SIZE, remainingToWrite);

                exe.ReadDataBlock(&mainMemory[PYSHICAL_ADDR(virtualPage)], PAGE_SIZE, offsetFile);
                pageTable[virtualPage].use = true;

                offsetFile += PAGE_SIZE; // Avanzamos en el Disco 
                remainingToWrite -= PAGE_SIZE; // Nos falta una página menos.

                // Avanzamos a la página siguiente
                virtualPage++;
            }

                DEBUG('a', "Write at last 0x%X, size %u\n",
                    PYSHICAL_ADDR(virtualPage) + offsetPage, remainingToWrite);

            // Ultima escritura, ya que remainingToWrite <= PAGE_SIZE
            exe.ReadDataBlock(&mainMemory[PYSHICAL_ADDR(virtualPage)], remainingToWrite, offsetFile);
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
        bit_map->Clear(pageTable[i].physicalPage);

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
            //printf("%d\n",tlb[i].virtualPage);
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

void 
AddressSpace::UpdateTLB(unsigned indexTlb, unsigned badVAddr) 
{
    
    unsigned badPageNumber = badVAddr / PAGE_SIZE;
    unsigned physicalPage = currentThread->space->pageTable[badPageNumber].physicalPage;
    MMU* MMU = machine->GetMMU(); 
    if (MMU->tlb[indexTlb].valid) 
    {
        unsigned pageNumber = MMU->tlb[indexTlb].virtualPage;
        //unsigned pagePhisical = MMU->tlb[indexTlb].physicalPage;
        bool valid = MMU->tlb[indexTlb].valid;
        bool use = MMU->tlb[indexTlb].use;
        bool dirty = MMU->tlb[indexTlb].dirty;
        bool readOnly = MMU->tlb[indexTlb].readOnly;    
        currentThread->space->pageTable[pageNumber].valid = valid;
        currentThread->space->pageTable[pageNumber].use = use;
        currentThread->space->pageTable[pageNumber].dirty = dirty;
        currentThread->space->pageTable[pageNumber].readOnly = readOnly;
    }
    MMU->tlb[indexTlb].virtualPage = badPageNumber;
    MMU->tlb[indexTlb].physicalPage = physicalPage;
    MMU->tlb[indexTlb].valid = currentThread->space->pageTable[badPageNumber].valid;
    MMU->tlb[indexTlb].use = currentThread->space->pageTable[badPageNumber].use;
    MMU->tlb[indexTlb].dirty = currentThread->space->pageTable[badPageNumber].dirty;
    MMU->tlb[indexTlb].readOnly = currentThread->space->pageTable[badPageNumber].readOnly;
}
