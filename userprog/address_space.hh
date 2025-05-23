/// Data structures to keep track of executing user programs (address
/// spaces).
///
/// For now, we do not keep any information about address spaces.  The user
/// level CPU state is saved and restored in the thread executing the user
/// program (see `thread.hh`).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_USERPROG_ADDRESSSPACE__HH
#define NACHOS_USERPROG_ADDRESSSPACE__HH


#include "coff_reader.h"
#include "filesys/file_system.hh"
#include "machine/translation_entry.hh"
#include "lib/table.hh"
#include "userprog/executable.hh"
#include <cstdint>


const unsigned USER_STACK_SIZE = 1024;  ///< Increase this as necessary!


class AddressSpace {
public:

    /// Create an address space to run a user program.
    ///
    /// The address space is initialized from an already opened file.
    /// The program contained in the file is loaded into memory and
    /// everything is set up so that user instructions can start to be
    /// executed.
    ///
    /// Parameters:
    /// * `executable_file` is the open file that corresponds to the
    ///   program; it contains the object code to load into memory.
    AddressSpace(OpenFile *executable_file, int newThreadPid);

    /// De-allocate an address space.
    ~AddressSpace();

    /// Initialize user-level CPU registers, before jumping to user code.
    void InitRegisters();

    /// Save/restore address space-specific info on a context switch.

    void SaveState();
    void RestoreState();

    /// Update Tlb
    void UpdateTLB(unsigned indexTlb, unsigned badVAddr);

    #ifdef DEMAND_LOADING
    // Using for DL
    void LoadPage(unsigned badVAddr);
    #endif
    #ifdef SWAP
    // Using for SWAP
    void Swap(unsigned vpn_to_store);
    void GetFromSwap(unsigned vpn);
    bool TestSwapMap(unsigned i);
    void WriteSwapFile(unsigned vpn, unsigned physicalPage);
    void ReadSwapFile(unsigned vpn, unsigned physicalPage);
    void MarkSwapMap(unsigned vpn);
    #endif

    // Método para actualizar la pageTable.
    void ActPageTable(unsigned virtualPage, unsigned physicalPage, bool valid, 
                      bool readOnly, bool use, bool dirty);
    
    // Métodos para obtener los campos de la pageTable.
    unsigned GetPageVpn(unsigned vpn);
    unsigned GetPagePhysicalPage(unsigned vpn);
    bool GetPageValid(unsigned vpn);
    bool GetPageReadOnly(unsigned vpn);
    bool GetPageUse(unsigned vpn);
    bool GetPageDirty(unsigned vpn);

    unsigned GetNumPages();
private:

    /// Assume linear page table translation for now!
    TranslationEntry *pageTable;

    static uint32_t GetPyshicalPage(uint32_t virtualAddr);

    /// Number of pages in the virtual address space. -> Para no guerdar una tabla de paginacion enorme.
    /// Si un proceso quiere acceder a un valor superior al numPages dará una excepción.
    unsigned numPages;
    
    // Pid del thread al cual pertenece este addresspace.
    int pid;

    #ifdef DEMAND_LOADING
    // Using for DL
        Executable *exe; 
        OpenFile *exe_file;
    #endif

    #ifdef SWAP
        char* swapName;
        OpenFile *swapFile;
        bool* swapMap;
    #endif
};

// Cada vez que se crea un proceso nuevo se crean 3/4 paginas para text/data y resto para stack.

#endif
