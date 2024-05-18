/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "executable.hh"
#include "lib/utility.hh"
#include "threads/system.hh"

#include <string.h>


/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *executable_file)
{
    ASSERT(executable_file != nullptr);
    
    fileTableIds = new Table <OpenFile*>;
    OpenFile* in = nullptr;
    OpenFile* out = nullptr;
    fileTableIds->Add(in);
    fileTableIds->Add(out);

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
    for (unsigned i = 0; i < numPages; i++) { /// Asigna 1:1 las páginas con la memoria fisica. -> Cambiar
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

    // Zero out the entire address space, to zero the unitialized data
    // segment and the stack segment.

    for (int i = 0; i < numPages; i++)
        memset(machine->mainMemory[pageTable[i].physicalPage], 0, PAGE_SIZE);

    // Then, copy in the code and data segments into memory.
    uint32_t codeSize = exe.GetCodeSize();
    uint32_t initDataSize = exe.GetInitDataSize();
    if (codeSize > 0) {
        uint32_t virtualAddr = exe.GetCodeAddr();
        //DEBUG('a', "Initializing code segment, at 0x%X, size %u\n",
              //virtualAddr, codeSize);

        // uint32_t ncpag = DivRoundUp(codeSize, PAGE_SIZE); <- Itero sobre esto, que serán menos ciclos.
        // Cargo el codigo en memoria. ReadCodeBlock llama a ReadAt y utiliza el offset para moverse dentro de las direcciones inFile.
        for (int i = 0; i < codeSize; i++){
            exe.ReadCodeBlock(&(mainMemory + (pageTable[virtualAddr / PAGE_SIZE].physicalPage)*PAGE_SIZE), PAGE_SIZE - (virtualAddr % PAGE_SIZE), virtualAddr % PAGE_SIZE);
            virtualAddr++;
        }
    }
    if (initDataSize > 0) {
        uint32_t virtualAddr = exe.GetInitDataAddr();
        DEBUG('a', "Initializing data segment, at 0x%X, size %u\n",
              virtualAddr, initDataSize);
        exe.ReadDataBlock(&mainMemory[virtualAddr], initDataSize, 0);
    }

}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
    delete [] pageTable;
    delete fileTableIds;
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
{}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void
AddressSpace::RestoreState()
{
    /// Comentar estas dos
    machine->GetMMU()->pageTable     = pageTable;
    machine->GetMMU()->pageTableSize = numPages;
    
    //Invalidar la TLB
    // for (int i = 0; i < TLB_SIZE; i++){
    //  machine->GetMMU()->tlb[i].valid=0;
    //
    // }

}
