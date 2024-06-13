/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "translation_entry.hh"
#include <complex.h>
#include <cstdint>
#define MIN(a,b) a < b ? a : b
#define MAX(a,b) a > b ? a : b
#define PHYSICAL_PAGE_ADDR(virtualPage) pageTable[virtualPage].physicalPage * PAGE_SIZE 

#include <cstring>
#include <stdio.h>
#include <stdlib.h>

#include "address_space.hh"
#ifndef USE_DL
#include "executable.hh"
#endif
#include "lib/utility.hh"
#include "mmu.hh"
#include "threads/system.hh"

/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
#ifndef USE_DL
AddressSpace::AddressSpace(OpenFile *executable_file)
#else
AddressSpace::AddressSpace(OpenFile *executable_file) : exe(executable_file)
#endif
{
    ASSERT(executable_file != nullptr);
    
    
    /// Creo el ejecutable
    #ifndef USE_DL
    Executable exe (executable_file);
    ASSERT(exe.CheckMagic());
    #else
    exe_file = executable_file;
    #endif
    
    unsigned size = exe.GetSize() + USER_STACK_SIZE; /// Lo que ocupa el .code + .data + .unidata + el stack.
    // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    DEBUG('y',"Numero de paginas %u.\n", numPages);
    size = numPages * PAGE_SIZE; /// Recalcula el nuevo tamaño con las páginas de mas incluidas.

    #ifdef SWAP
    swapname = concat("SWAP.", std::to_string(currentThread->GetPid()));
    ASSERT(Create(swapname, size));
    #else 
    ASSERT(numPages <= bit_map->CountClear()); /// Calculamos la cantidad de espacio disponible según el bitmap      // Check we are not trying to run anything too big -- at least until we
      // have virtual memory.
    #endif

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.
    
    // Al momento de usar páginas, marcarlas como usada. Cuando se terminan de usar, marcarlas como libre.
    pageTable = new TranslationEntry[numPages]; /// Crea la tabla de paginacion
    for (unsigned int i = 0; i < numPages; i++) { /// Asigna 1:1 las páginas con la memoria fisica. -> Cambiar
        pageTable[i].virtualPage  = i;
          /// Devolvemos el primer lugar de la memoria física libre. 
        #ifndef USE_DL
        pageTable[i].physicalPage = bit_map->Find();
        pageTable[i].valid        = true;
        #else
        pageTable[i].physicalPage = -1;
        pageTable[i].valid        = false;
        #endif
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
          // If the code segment was entirely on a separate page, we could
          // set its pages to be read-only.
    }

    #ifndef USE_DL
    char *mainMemory = machine->mainMemory; /// mainMemory es un arreglo de bytes.

    // Zero out the entire address space, to zero the unitialized data
    // segment and the stack segment.

    DEBUG('a', "Seteando en 0 la memoria de las páginas.\n");
    for (unsigned i = 0; i < numPages; i++)
        memset(&mainMemory[PHYSICAL_PAGE_ADDR(i)], 0, PAGE_SIZE);

    // Then, copy in the code and data segments into memory.
    uint32_t codeSize = exe.GetCodeSize();
    if (codeSize > 0) {

        uint32_t virtualAddr = exe.GetCodeAddr();
        DEBUG('a', "Initializing code segment, at 0x%X, size %u\n",
              virtualAddr, codeSize);
        uint32_t offsetPage = virtualAddr % PAGE_SIZE;
        uint32_t offsetFile = 0;
        unsigned virtualPage = virtualAddr / PAGE_SIZE;
        uint32_t firstPageWriteSize = MIN(PAGE_SIZE - offsetPage, codeSize);
        uint32_t remainingToWrite = codeSize - firstPageWriteSize;


        DEBUG('a', "Write at 0x%X, size %u\n",
              PHYSICAL_PAGE_ADDR(virtualPage) + offsetPage, firstPageWriteSize);


        exe.ReadCodeBlock(
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

                exe.ReadCodeBlock(&mainMemory[PHYSICAL_PAGE_ADDR(virtualPage)], PAGE_SIZE, offsetFile);
                pageTable[virtualPage].use = true;

                offsetFile += PAGE_SIZE; // Avanzamos en el Disco 
                remainingToWrite -= PAGE_SIZE; // Nos falta una página menos.

                // La página es solo del segmento del código, marcamos como solo lectura.
                pageTable[virtualPage].readOnly = true; 

                // Avanzamos a la página siguiente
                virtualPage++;
            }

                DEBUG('a', "Last Write at 0x%X, size %u\n",
                    PHYSICAL_PAGE_ADDR(virtualPage), remainingToWrite);
            // Ultima escritura, ya que remainingToWrite <= PAGE_SIZE
            exe.ReadCodeBlock(&mainMemory[PHYSICAL_PAGE_ADDR(virtualPage)], remainingToWrite, offsetFile);
            pageTable[virtualPage].use = true;

            //Si la útima escritura es una página entera, marcamos la página como solo lecutura.
            if (remainingToWrite == PAGE_SIZE)
                pageTable[virtualPage].readOnly = true;
        }

    }

    uint32_t initDataSize = exe.GetInitDataSize();

    if (initDataSize > 0) {

        uint32_t virtualAddr = exe.GetInitDataAddr();
        DEBUG('a', "Initializing data segment, at 0x%X, size %u\n",
              virtualAddr, initDataSize);
        uint32_t offsetPage = virtualAddr % PAGE_SIZE;
        uint32_t offsetFile = 0;
        unsigned virtualPage = virtualAddr / PAGE_SIZE;
        uint32_t firstPageWriteSize = MIN(PAGE_SIZE - offsetPage, initDataSize);
        uint32_t remainingToWrite = initDataSize - firstPageWriteSize;

        DEBUG('a', "Write at 0x%X, size %u, offsetPage %u, remainingToWrite %u\n",
              PHYSICAL_PAGE_ADDR(virtualPage) + offsetPage, firstPageWriteSize, offsetPage, remainingToWrite);

        exe.ReadDataBlock(&mainMemory[PHYSICAL_PAGE_ADDR(virtualPage) + offsetPage], firstPageWriteSize, offsetFile);
        pageTable[virtualPage].use = true;
        if (remainingToWrite > 0) {
            offsetFile = firstPageWriteSize; // Desplazamiento en Disco
            virtualPage++;                   // Pasamos a la siguiente página virtual

            // Si lo que falta escribir supera el tamaño de una página,
            // tenemos que escribir una página entera
            while(PAGE_SIZE < remainingToWrite) 
            {
                DEBUG('a', "Write in while at 0x%X, size %u, remainingToWrite\n",
                    PHYSICAL_PAGE_ADDR(virtualPage), PAGE_SIZE, remainingToWrite);

                exe.ReadDataBlock(&mainMemory[PHYSICAL_PAGE_ADDR(virtualPage)], PAGE_SIZE, offsetFile);
                pageTable[virtualPage].use = true;

                offsetFile += PAGE_SIZE; // Avanzamos en el Disco 
                remainingToWrite -= PAGE_SIZE; // Nos falta una página menos.

                // Avanzamos a la página siguiente
                virtualPage++;
            }

                DEBUG('a', "Write at last 0x%X, size %u\n",
                    PHYSICAL_PAGE_ADDR(virtualPage) + offsetPage, remainingToWrite);

            // Ultima escritura, ya que remainingToWrite <= PAGE_SIZE
            exe.ReadDataBlock(&mainMemory[PHYSICAL_PAGE_ADDR(virtualPage)], remainingToWrite, offsetFile);
            pageTable[virtualPage].use = true;
        }
    }
    #endif
}

/// Deallocate an address space.
AddressSpace::~AddressSpace()
{
    for (unsigned i = 0; i < numPages; i++)
        if (pageTable[i].valid)
            bit_map->Clear(pageTable[i].physicalPage);

    #ifdef USE_DL
    delete exe_file;
    #endif

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
    #ifdef USE_TLB
    TranslationEntry to_cpy;
    for(unsigned int i = 0; i < TLB_SIZE; i++){
        to_cpy = machine->GetMMU()->tlb[i];
        if (to_cpy.valid && !to_cpy.readOnly)
            this->CommitPage(to_cpy);
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
    for (unsigned i = 0; i < TLB_SIZE; i++){
        machine->GetMMU()->tlb[i].valid = false;
    }
    #else
    machine->GetMMU()->pageTable     = pageTable;
    machine->GetMMU()->pageTableSize = numPages;
    #endif
}
void
AddressSpace::CommitPage(TranslationEntry newTransEntry)
{
    pageTable[newTransEntry.virtualPage] = newTransEntry;
    return;
}    
TranslationEntry 
AddressSpace::GetPage(unsigned vpn)
{

    #ifdef USE_DL
    if (pageTable[vpn].valid == false){
        
        #ifndef SWAP /// Bit de swap <- si ya se swapeó antes o no
        pageTable[vpn].physicalPage = bit_map->Find();
        #else
        pageTable[vpn].physicalPage = CoreMap->Find(vpn, currentThread->proc_id);
        #endif

        char* mainMemory = machine->mainMemory;
        DEBUG('y',"Invalid Page, start loading process.\n");

        // Direccion virtual del incio de la página
        unsigned pageDownAddress = vpn * PAGE_SIZE; 

        // Direccion de la siguiente página (tope de la página actual)
        unsigned pageUpAddress = pageDownAddress + PAGE_SIZE;

        unsigned codeAddr = exe.GetCodeAddr();          //Inicio segmento código
        unsigned codeSize = exe.GetCodeSize();          //Tamaño del segmento de código
        unsigned dataAddr = exe.GetInitDataAddr();      //Inicio segmento datos
        unsigned dataSize = exe.GetInitDataSize();      //Tamaño del segmento de datos

        DEBUG('y', "Write vp  %u, initAddr %u, size %u\n",
            vpn, PHYSICAL_PAGE_ADDR(vpn), PAGE_SIZE); 

        memset(&mainMemory[PHYSICAL_PAGE_ADDR(vpn)], 0, PAGE_SIZE);

        //Necesito escribir algo del segmento de código
        if (codeSize > 0 && pageDownAddress < codeAddr + codeSize){ 
            //Parte de direcciones virtuales
            unsigned firstAddre = MAX(pageDownAddress, codeAddr); // En nachos podríamos dejar solo pageDownAddress
            unsigned offSet = firstAddre % PAGE_SIZE;
            unsigned bytesToWrite = MIN(codeAddr + codeSize - firstAddre, pageUpAddress - firstAddre);

            //Si lo que escribimos en el segmento de código es un página entera,
            //entonces la página se puede marcar como read only
            if (bytesToWrite == PAGE_SIZE)
                pageTable[vpn].readOnly = true;

            //Parte el archivo del ejecutable
            unsigned fileOffSet = firstAddre - codeAddr;

            //Escritura en la página física
            exe.ReadCodeBlock(&mainMemory[PHYSICAL_PAGE_ADDR(vpn) + offSet], bytesToWrite, fileOffSet);
        }

        //Necesito escribir algo del segmento de datos inicializados
        if (dataSize > 0 && dataAddr < pageUpAddress && pageDownAddress < dataAddr + dataSize){
            //Parte de direcciones virtuales
            unsigned firstAddre = MAX(pageDownAddress, dataAddr); //Acá es necesario el max
            unsigned offSet = firstAddre % PAGE_SIZE;
            unsigned bytesToWrite = MIN(dataAddr + dataSize - firstAddre, pageUpAddress - firstAddre);

            //Parte el archivo del ejecutable
            unsigned fileOffSet = firstAddre - dataAddr;

            //Escritura en la página física
            exe.ReadDataBlock(&mainMemory[PHYSICAL_PAGE_ADDR(vpn) + offSet], bytesToWrite, fileOffSet);
        }

        pageTable[vpn].valid = true;
        pageTable[vpn].use = true;
    }
    #endif
    return pageTable[vpn];
}
