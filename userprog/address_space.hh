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

#include "filesys/file_system.hh"
#include "machine/translation_entry.hh"
#include "lib/table.hh"
#ifdef USE_DL
#include "executable.hh"
#endif
#ifdef SWAP
#include "lib/bitmap.hh"
#endif
#include <cstdint>

class Thread;

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
    AddressSpace(OpenFile *executable_file);

    /// De-allocate an address space.
    ~AddressSpace();

    /// Initialize user-level CPU registers, before jumping to user code.
    void InitRegisters();

    /// Save/restore address space-specific info on a context switch.

    void SaveState();
    
    void RestoreState();

    void GetPage(unsigned vpn, TranslationEntry* tlb_entry);
    
#ifdef SWAP 
    void SetIdxTLB(unsigned vpn, unsigned idx);

    void LetSwap(unsigned vpn);
    
    void GetSwap(unsigned ppn);
    
    void Swapping(unsigned vpn);
#endif 

    void CommitPage(TranslationEntry* newTransEntry); 
    
    void RetrievePage(unsigned vpn);

    Table <OpenFile*> *fileTableIds;

private:
    
    TranslationEntry *pageTable;
    
    #ifdef USE_DL
    OpenFile *exe_file;
    Executable* exe;
    #endif
    
    #ifdef SWAP
    char* swapname;
    OpenFile *swap_pid;
    Bitmap* swap_map;
    unsigned* idx_tlb;
    #endif

    /// Assume linear page table translation for now!

    /// Number of pages in the virtual address space. -> Para no guerdar una tabla de paginacion enorme.
    /// Si un proceso quiere acceder a un valor superior al numPages dará una excepción.
    unsigned numPages;
};

// Cada vez que se crea un proceso nuevo se crean 3/4 paginas para text/data y resto para stack.

#endif
