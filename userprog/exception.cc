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
#include "filesys/open_file.hh"
#include "transfer.hh"
#include "syscall.h"
#include "filesys/directory_entry.hh"
#include "threads/system.hh"
#include "args.hh"

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

/// Run a user program.
///
/// Open the executable, load it into memory, and jump to it.
void
ProcessInitArgs(void* arg)
{
    int sp;

    currentThread->space->InitRegisters();  // Set the initial register values.
    currentThread->space->RestoreState();   // Load page table register.

    char** argv = (char**)arg;

    int c = WriteArgs(argv);

    //sp tiene el puntero a argv[]
    sp = machine->ReadRegister(STACK_REG);

    machine->WriteRegister(4, c);
    machine->WriteRegister(5, sp);

    //Le hacemos caso a args.hh
    machine->WriteRegister(STACK_REG, sp - 24);

    machine->Run();  // Jump to the user progam.
    ASSERT(false);   // `machine->Run` never returns; the address space
                     // exits by doing the system call `Exit`.
}

void
ProcessInit(void* arg)
{
    currentThread->space->InitRegisters();  // Set the initial register values.
    currentThread->space->RestoreState();   // Load page table register.

    machine->Run();  // Jump to the user progam.
    ASSERT(false);   // `machine->Run` never returns; the address space
                     // exits by doing the system call `Exit`.
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
            char filename[FILE_NAME_MAX_LEN + 1];

            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                status = -1;
            }

            if (!status && !ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = -1;
            }

            if (!status){
            DEBUG('e', "`Create` requested for file `%s`.\n", filename);
            
            unsigned initialSize = machine->ReadRegister(5);
            status = fileSystem->Create(filename, initialSize) ? 0 : -1;
            }
            
            machine->WriteRegister(2, status);
            break;
        }
        
        case SC_OPEN: {

            int filenameAddr = machine->ReadRegister(4);
            int status = 0;
            OpenFile *newFile;
            char filename[FILE_NAME_MAX_LEN + 1];

            if (filenameAddr == 0){
                DEBUG('e', "Error: address to filename string is null. \n");
                status = -1;
            }

            if (!status && !ReadStringFromUser(filenameAddr, 
                                    filename, sizeof filename)){
                 DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = -1;
            }
        
            if (!status && !(newFile = fileSystem->Open(filename))){
                DEBUG('e', "Cannot open file %s.\n", filename);
                status = -1; 
            }

            if (!status){
                DEBUG('e', "`Open` requested for file `%s`.\n", filename);
                status = currentThread->fileTableIds->Add(newFile);
            }
            machine->WriteRegister(2, status);
            break;
        } 
        
        case SC_CLOSE: {
            
            OpenFileId fid = machine->ReadRegister(4);
            OpenFile *file;
            int status = 0;
            DEBUG('e', "`Close` requested for id %u.\n", fid);

            if (fid == 0 || fid == 1){
                DEBUG('e', "Cannot close console fd id %u.\n", fid);
                status = -1;
            }
            
            if (!status && fid < 0){
                DEBUG('e', "Bad fd id %u.\n", fid);
                status = -1;
            }
            // Sacarlo de la tabla
            if (!status && !(file = currentThread->fileTableIds->Remove(fid))){
                DEBUG('e', "Cannot found fd id %u in table.\n", fid);
                status = -1;
            }
            
            // Sacarlo de memoria 
            if (!status)
                delete file;

            machine->WriteRegister(2, 0);
            break;
        } 
        
        case SC_REMOVE: {

            int filenameAddr = machine->ReadRegister(4);
            int status = 0;
            char filename[FILE_NAME_MAX_LEN + 1];

            if (filenameAddr == 0){
                DEBUG('e', "Error: address to filename string is null. \n");
                status = -1;
            }

            if (!status && !ReadStringFromUser(filenameAddr, 
                                    filename, sizeof filename)){
                 DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = -1;
            }

            if (!status)
                status = fileSystem->Remove(filename) ? 0 : -1;

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

            if (!status && bufferToWrite == 0){
                DEBUG('e', "Error: Buffer to write is null. \n");
                status = -1;
            }

            if (!status && bytesToRead < 0) {
                DEBUG('e', "Error: Bytes to read is negative. \n");
                status = -1;
            }
            
            if(!status && id != 0 && !(file = currentThread->fileTableIds->Get(id))) {
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
                
                if (status != 0) // NO estoy en EOF
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
            if(!status && id != 1 && !(file = currentThread->fileTableIds->Get(id))) {
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
        case SC_EXEC:{

            int filenameAddr = machine->ReadRegister(4); 
            int join = machine->ReadRegister(5); 
            int status = 0;
            OpenFile *executable;
            Thread* newThread;            
            AddressSpace *space; 
            char filename[FILE_NAME_MAX_LEN + 1];

            if (filenameAddr == 0){
                DEBUG('e', "Error: address to filename string is null. \n");
                status = -1;
            }

            if (!status && !ReadStringFromUser(filenameAddr, 
                                    filename, sizeof filename)){
                 DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = -1;
            }

            if (!status && !(executable = fileSystem->Open(filename))) {
                DEBUG('e', "Error: Unable to open file %s\n", filename);
                status = -1;
            }

            if (!status &&  !(newThread = new Thread(nullptr,join ? true : false))){
                DEBUG('e', "Error: Unable to create a thread %s\n", filename);
                status = -1; 
            }

            if (!status &&  !(space = new AddressSpace(executable))){
                DEBUG('e', "Error: Unable to create the address space \n");
                status = -1;
            }

            if (!status){
                delete executable;
                newThread->space = space;
                status = space_table->Add(newThread);
                newThread->Fork(ProcessInit, nullptr);
            }

            machine->WriteRegister(2, status);
            break;
        } 
        case SC_EXEC2:{

            int filenameAddr = machine->ReadRegister(4); 
            int argsVector = machine->ReadRegister(5);
            int join = machine->ReadRegister(6);
            int status = 0;
            OpenFile *executable;
            Thread* newThread;            
            char** argv;
            AddressSpace *space; 
            char filename[FILE_NAME_MAX_LEN + 1];

            if (filenameAddr == 0){
                DEBUG('e', "Error: address to filename string is null. \n");
                status = -1;
            }

            if (!status && argsVector == 0){
                DEBUG('e', "Error: address to argsVector is null. \n");
                status = -1;
            }

            if (!status && !ReadStringFromUser(filenameAddr, 
                                    filename, sizeof filename)){
                 DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = -1;
            }

            if (!status && !(argv = SaveArgs(argsVector))){
                 DEBUG('e', "Error: Unable to get User Args Vectors.\n",
                      FILE_NAME_MAX_LEN);
                status = -1;
            }

            if (!status && !(executable = fileSystem->Open(filename))) {
                DEBUG('e', "Error: Unable to open file %s\n", filename);
                status = -1;
            }

            if (!status &&  !(newThread = new Thread(nullptr, join ? true : false))){
                DEBUG('e', "Error: Unable to create a thread %s\n", filename);
                status = -1; 
            }

            if (!status &&  !(space = new AddressSpace(executable))){
                DEBUG('e', "Error: Unable to create the address space \n");
                status = -1;
            }

            if (!status){
                delete executable;
                newThread->space = space;
                status = space_table->Add(newThread);
                newThread->Fork(ProcessInitArgs, (void*)argv);
            }

            machine->WriteRegister(2, status);
            break;
        }
        case SC_EXEC:{

            int filenameAddr = machine->ReadRegister(4); 
            int join = machine->ReadRegister(5); 
            int status = 0;
            OpenFile *executable;
            Thread* newThread;            
            AddressSpace *space; 
            char filename[FILE_NAME_MAX_LEN + 1];

            if (filenameAddr == 0){
                DEBUG('e', "Error: address to filename string is null. \n");
                status = -1;
            }

            if (!status && !ReadStringFromUser(filenameAddr, 
                                    filename, sizeof filename)){
                 DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = -1;
            }

            if (!status && !(executable = fileSystem->Open(filename))) {
                DEBUG('e', "Error: Unable to open file %s\n", filename);
                status = -1;
            }

            if (!status &&  !(newThread = new Thread(nullptr,join ? true : false))){
                DEBUG('e', "Error: Unable to create a thread %s\n", filename);
                status = -1; 
            }

            if (!status &&  !(space = new AddressSpace(executable))){
                DEBUG('e', "Error: Unable to create the address space \n");
                status = -1;
            }

            if (!status){
                delete executable;
                newThread->space = space;
                status = space_table->Add(newThread);
                newThread->Fork(ProcessInit, nullptr);
            }

            machine->WriteRegister(2, status);
            break;
        } 
        case SC_EXEC2:{

            int filenameAddr = machine->ReadRegister(4); 
            int argsVector = machine->ReadRegister(5);
            int join = machine->ReadRegister(6);
            int status = 0;
            OpenFile *executable;
            Thread* newThread;            
            char** argv;
            AddressSpace *space; 
            char filename[FILE_NAME_MAX_LEN + 1];

            if (filenameAddr == 0){
                DEBUG('e', "Error: address to filename string is null. \n");
                status = -1;
            }

            if (!status && argsVector == 0){
                DEBUG('e', "Error: address to argsVector is null. \n");
                status = -1;
            }

            if (!status && !ReadStringFromUser(filenameAddr, 
                                    filename, sizeof filename)){
                 DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = -1;
            }

            if (!status && !(argv = SaveArgs(argsVector))){
                 DEBUG('e', "Error: Unable to get User Args Vectors.\n",
                      FILE_NAME_MAX_LEN);
                status = -1;
            }

            if (!status && !(executable = fileSystem->Open(filename))) {
                DEBUG('e', "Error: Unable to open file %s\n", filename);
                status = -1;
            }

            if (!status &&  !(newThread = new Thread(nullptr, join ? true : false))){
                DEBUG('e', "Error: Unable to create a thread %s\n", filename);
                status = -1; 
            }

            if (!status &&  !(space = new AddressSpace(executable))){
                DEBUG('e', "Error: Unable to create the address space \n");
                status = -1;
            }

            if (!status){
                delete executable;
                newThread->space = space;
                status = space_table->Add(newThread);
                newThread->Fork(ProcessInitArgs, (void*)argv);
            }

            machine->WriteRegister(2, status);
            break;
        } 
        case SC_EXIT: {

            int ret = machine->ReadRegister(4);            

            delete currentThread->space;


            if (space_table->Get(0) == currentThread) // Main thread Exit
                interrupt->Halt();

            currentThread->Finish(ret); 
            break;
        }
        case SC_JOIN: {

            SpaceId childId = machine->ReadRegister(4); 
            int status = 0;
            Thread *child;
            if (!(child = space_table->Get(childId))){
                DEBUG('e', "Error: Unable to get the childId.\n");
                status = -1;
            }
            if (!status){
                space_table->Remove(childId);
                status = child->Join();
            }
            
            machine->WriteRegister(2, status);
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
