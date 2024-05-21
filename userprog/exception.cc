/// Entry points into the Nachos kernel from user programs.
///
/// There are two kinds of things that can cause control to transfer back to
/// here from user code:
///
/// * System calls: the user code explicitly requests to call a procedure in
///   the Nachos kernel.  Right now, the only function we support is `Halt`.
///
/// * Exceptions: the user code does something that the CPU cannot handle.
///   For instance, accessing memory that does not exist, arithmetic errors,
///   etc.
///
/// Interrupts (which can also cause control to transfer from user code into
/// the Nachos kernel) are handled elsewhere.
///
/// For now, this only handles the `Halt` system call.  Everything else core-
/// dumps.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "filesys/file_system.hh"
#include "lib/assert.hh"
#include "synch_console.hh"
#include "threads/thread.hh"
#include "transfer.hh"
#include "syscall.h"
#include "filesys/directory_entry.hh"
#include "threads/system.hh"
#include "address_space.hh"

#include <stdio.h>


static void
IncrementPC()
{
    unsigned pc;

    pc = machine->ReadRegister(PC_REG);
    machine->WriteRegister(PREV_PC_REG, pc);
    pc = machine->ReadRegister(NEXT_PC_REG);
    machine->WriteRegister(PC_REG, pc);
    pc += 4;
    machine->WriteRegister(NEXT_PC_REG, pc);
}

void
StartProcess(void *filename)
{
    
    currentThread->space->InitRegisters();  // Set the initial register values.
    currentThread->space->RestoreState();   // Load page table register.

    machine->Run();  // Jump to the user progam.
    ASSERT(false);   // `machine->Run` never returns; the address space
                     // exits by doing the system call `Exit`.
}



/// Do some default behavior for an unexpected exception.
///
/// NOTE: this function is meant specifically for unexpected exceptions.  If
/// you implement a new behavior for some exception, do not extend this
/// function: assign a new handler instead.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
static void
DefaultHandler(ExceptionType et) /// Cambia por PageFaultHandler. No incrementar el PC. Cuestion es: de donde sacar la dirección => VPN que fallo? De un registro simulado machine->register[BadVAddr]
{
    int exceptionArg = machine->ReadRegister(2);

    fprintf(stderr, "Unexpected user mode exception: %s, arg %d.\n",
            ExceptionTypeToString(et), exceptionArg);
    ASSERT(false);
}

/// Handle a system call exception.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
///
/// The calling convention is the following:
///
/// * system call identifier in `r2`;
/// * 1st argument in `r4`;
/// * 2nd argument in `r5`;
/// * 3rd argument in `r6`;
/// * 4th argument in `r7`;
/// * the result of the system call, if any, must be put back into `r2`.
///
/// And do not forget to increment the program counter before returning. (Or
/// else you will loop making the same system call forever!)
static void
SyscallHandler(ExceptionType _et)
{
    int scid = machine->ReadRegister(2);

    switch (scid) {

        case SC_HALT:
            DEBUG('e', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
            break;

        case SC_CREATE: {
            int filenameAddr = machine->ReadRegister(4);
            int status = 0;

            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                status = -1;
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!status && !ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = -1;
            }

            if (!status){
            DEBUG('e', "`Create` requested for file `%s`.\n", filename);
            
            unsigned initialSize = machine->ReadRegister(5);
            fileSystem->Create(filename, initialSize);
            }
            
            machine->WriteRegister(2, status);
            break;
        }
        
        case SC_OPEN: {
            int filenameAddr = machine->ReadRegister(4);
            int status = 0;
            OpenFile *newFile;

            if (filenameAddr == 0){
                DEBUG('e', "Error: address to filename string is null. \n");
                status = -1;
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!status && !ReadStringFromUser(filenameAddr, 
                                    filename, sizeof filename)){
                 DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = -1;
            }
        
            if (!status){
                DEBUG('e', "`Open` requested for file `%s`.\n", filename);
                newFile = fileSystem->Open(filename);
                status = currentThread->space->fileTableIds->Add(newFile);
            }

            machine->WriteRegister(2, status);
            break;
        } 
        
        case SC_CLOSE: {
            
            OpenFileId fid = machine->ReadRegister(4);
            DEBUG('e', "`Close` requested for id %u.\n", fid);
            
            // Buscar en tabla
            OpenFile *file = currentThread->space->fileTableIds->Remove(fid);
            
            // Sacarlo de la tabla
            delete file;

            machine->WriteRegister(2, 0);
            break;
        } 
        
        case SC_REMOVE: {
            int filenameAddr = machine->ReadRegister(4);
            int status = 0;
            if (filenameAddr == 0){
                DEBUG('e', "Error: address to filename string is null. \n");
                status = -1;
            }
            char filename[FILE_NAME_MAX_LEN + 1];
            if (!status && !ReadStringFromUser(filenameAddr, 
                                    filename, sizeof filename)){
                 DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = -1;
            }

            if (!status)
                fileSystem->Remove(filename);

            machine->WriteRegister(2, status);
            break;
        } 

        case SC_READ: {
            
            int bufferToWrite = machine->ReadRegister(4);
            int bytesToRead = machine->ReadRegister(5);
            OpenFileId id = machine->ReadRegister(6);
            OpenFile *file;
            int status = 0;
            char bufferTransfer[bytesToRead];
            
            if (id == 1){
                DEBUG('e', "Error: File Descriptor Stdout");
                status = -1;
            }

            if (bufferToWrite == 0){
                DEBUG('e', "Error: Buffer to write is null. \n");
                status = -1;
            }
            if (!status && bytesToRead < 0) {
                DEBUG('e', "Error: Bytes to read is negative. \n");
                status = -1;
            }
            
            // Buscar id
            
            if(!status && id != 0 && !(file = currentThread->space->fileTableIds->Get(id))) {
                DEBUG('e', "Error: File not found. \n");
                status = -1;
            }
            
            if (!status) {
                if (id == 0){
                    status = bytesToRead;
                    synch_console->ReadNBytes(bufferTransfer, bytesToRead);
                }
                else
                    status = file->Read(bufferTransfer, bytesToRead);
                
                DEBUG('a', "Success: Read done\n");
                WriteBufferToUser(bufferTransfer, bufferToWrite, status);
            }
            
            machine->WriteRegister(2, status);
            break;       
        }

        case SC_WRITE: {
            
            int bufferToRead = machine->ReadRegister(4);
            int bytesToWrite = machine->ReadRegister(5);
            OpenFileId id = machine->ReadRegister(6);
            int status = 0;            
            OpenFile *file;            
            char bufferTransfer[bytesToWrite];

            if (id == 0){
                DEBUG('e', "Error: File Descriptor Stdin");
                status = -1;
            }
                
            if (!status && bufferToRead == 0){
                DEBUG('e', "Error: Buffer to read is null. \n");
                status = -1;
            }

            if (!status && bytesToWrite <= 0) {
                DEBUG('e', "Error: Bytes to write is non positive. \n");
                status = -1;
            }
            
            //Buscar id
            if(!status && id != 1 && !(file = currentThread->space->fileTableIds->Get(id))) {
                DEBUG('e', "Error: File not found. \n");
                status = -1;
            }
            
            if (!status) {
            ReadBufferFromUser(bufferToRead, bufferTransfer, bytesToWrite);
                if (id == 1){
                    status = bytesToWrite;
                    synch_console->WriteNBytes(bufferTransfer, bytesToWrite); 
                }
                else 
                    status = file->Write(bufferTransfer, bytesToWrite); 
            }

            DEBUG('a', "Success: Write done\n");
            machine->WriteRegister(2, status);
            break;
        
        }
        case SC_EXEC: {
                        
            // Cargamos el nombre del ejecutable.
            int filenameAddr = machine->ReadRegister(4);
    
            if (filenameAddr == 0)
                DEBUG('e', "Error: address to filename string is null. \n");

            char filename[FILE_NAME_MAX_LEN + 1];

            if (!ReadStringFromUser(filenameAddr, 
                                    filename, sizeof filename))
                 DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
            
            OpenFile *executable = fileSystem->Open((char*) filename);
            
            if (executable == nullptr) {
                printf("Unable to open file %s\n", (char*) filename);
                machine->WriteRegister(2,-1);
                break;
            }

            AddressSpace *space = new AddressSpace(executable);

            Thread* child = new Thread("execChild", true);
            
            child->space = space;
            delete executable;
            
            child->Fork(StartProcess, nullptr);
            machine->WriteRegister(2, currentThread->space->spaceTable->Add(child));
            
            break;
        }
        case SC_JOIN: {

            DEBUG('a', "Thread Join\n");
            SpaceId child = machine->ReadRegister(2);
             
            /// Hago que el padre ejecute el método join del hijo.
            Thread* childJoin = currentThread->space->spaceTable->Get(child);

            if(childJoin != nullptr){
                childJoin->Join();
                machine->WriteRegister(2, currentThread->resp);
            }
            else 
                machine->WriteRegister(2, -1);
                       
            
            DEBUG('a', "Thread Join End\n");
            break;
        }
        case SC_EXIT: {
           int status = machine->ReadRegister(2);
           
            currentThread->Finish(status);
        
            break;
        } 
        default:
            fprintf(stderr, "Unexpected system call: id %d.\n", scid);
            ASSERT(false);

    }

    IncrementPC();
}


/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void
SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION,            &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION,       &SyscallHandler);
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &DefaultHandler); /// Cambiar el manejador por PageFaultHandler
    machine->SetHandler(READ_ONLY_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION,      &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}
