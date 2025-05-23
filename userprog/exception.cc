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
#include "filesys/open_file.hh"
#include "transfer.hh"
#include "syscall.h"
#include "filesys/directory_entry.hh"
#include "threads/system.hh"
#include "args.hh"

#include <stdio.h>

unsigned IndexTLB = 0;

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
    printf("Di error, soy: %d\n", currentThread->GetPid());
    fprintf(stderr, "Unexpected user mode exception: %s, arg %d.\n",
            ExceptionTypeToString(et), exceptionArg);
    ASSERT(false);
}

#ifdef USE_TLB
static void
PageFaultHandler(ExceptionType et) 
{
    unsigned badVAddr = machine->ReadRegister(BAD_VADDR_REG);
    currentThread->space->UpdateTLB(IndexTLB, badVAddr);
    IndexTLB = (IndexTLB + 1) % TLB_SIZE;
}
#endif

/// Run a user program.
///
/// Open the executable, load it into memory, and jump to it.
void
ProcessInitArgs(void* arg)
{
    int sp;
    
    int newpid;
    newpid = space_table->Add(currentThread);
    currentThread->SetPid(newpid);
    //printf("newpid en ProcessInitArgs: %d\n", newpid);
    ASSERT(newpid != -1);

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
    int newpid;
    newpid = space_table->Add(currentThread);
    currentThread->SetPid(newpid);
    //printf("newpid en ProcessInit: %d\n", newpid);
    ASSERT(newpid != -1);
    
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
            printf("HALT: %s\n", currentThread->GetName());
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

            machine->WriteRegister(2, status);
            break;
        }
        case SC_EXEC:{

            int filenameAddr = machine->ReadRegister(4); 
            int join = machine->ReadRegister(5); 
            bool status = true;
            OpenFile *executable;
            Thread* newThread;            
            AddressSpace *space; 
            char filename[FILE_NAME_MAX_LEN + 1];

            if (filenameAddr == 0){
                puts("Error filename null");
                DEBUG('e', "Error: address to filename string is null. \n");
                status = false;
            }

            if (!ReadStringFromUser(filenameAddr, 
                                    filename, sizeof filename) || !status){
                puts("Error filename long"); 
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = false;
            }

            if (!(executable = fileSystem->Open(filename)) || !status) {
                puts("Error al abrir el archivo");
                DEBUG('e', "Error: Unable to open file %s\n", filename);
                status = false;
            }

            if (!(newThread = new Thread(nullptr,join ? true : false)) || !status){
                puts("Error al crear el thread");
                DEBUG('e', "Error: Unable to create a thread %s\n", filename);
                status = false; 
            }
            
            int newpid = -1;
            if (status)
                newpid = space_table->Add(newThread);
            
            if (newpid == -1 && status)
            {
                puts("Error agg thread space table");
                DEBUG('e', "Error: No se puede agregar el Thread a la space_table\n");
                status = false;
            }

            if (!(space = new AddressSpace(executable, newpid)) || !status){
                puts("Error crear addres space\n"); 
                DEBUG('e', "Error: Unable to create the address space \n");
                status = false;
            }

            // Caso en que falló la creación del AddressSpace pero el thread se agregó a la tabla.
            if (newpid != -1 && !status)
                space_table->Remove(newpid);

            if (status){
                // No creo que tenga sentido borrar el ejecutable.
                //delete executable;
                newThread->SetPid(newpid);
                newThread->space = space;
                newThread->Fork(ProcessInit, nullptr);
            }
            else{
                if (newThread != nullptr)
                    delete newThread;
                if (space != nullptr)
                    delete space;
            }

            machine->WriteRegister(2, newpid);
            break;
        } 
        case SC_EXEC2:{

            int filenameAddr = machine->ReadRegister(4); 
            int argsVector = machine->ReadRegister(5);
            int join = machine->ReadRegister(6);
            bool status = true;
            OpenFile *executable;
            Thread* newThread;            
            char** argv;
            AddressSpace *space; 
            char filename[FILE_NAME_MAX_LEN + 1];

            if (filenameAddr == 0){
                DEBUG('e', "Error: address to filename string is null. \n");
                status = false;
            }

            if (!status || argsVector == 0){
                DEBUG('e', "Error: address to argsVector is null. \n");
                status = false;
            }

            if (!ReadStringFromUser(filenameAddr, 
                                    filename, sizeof filename) || !status){
                 DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = false;
            }

            if (!(argv = SaveArgs(argsVector)) || !status){
                 DEBUG('e', "Error: Unable to get User Args Vectors.\n",
                      FILE_NAME_MAX_LEN);
                status = false;
            }

            if (!(executable = fileSystem->Open(filename)) || !status) {
                DEBUG('e', "Error: Unable to open file %s\n", filename);
                status = false;
            }

            if (!(newThread = new Thread(nullptr, join ? true : false)) || !status){
                DEBUG('e', "Error: Unable to create a thread %s\n", filename);
                status = false; 
            }

            int newpid = -1;
            if (status)
                newpid = space_table->Add(newThread);
            
            if (newpid == -1 && status)
            {
                DEBUG('e', "Error: No se puede agregar el Thread a la space_table\n");
                status = false;
            }

            if (!(space = new AddressSpace(executable, newpid)) || !status){
                DEBUG('e', "Error: Unable to create the address space \n");
                status = false;
            }

            // Caso en que falló la creación del AddressSpace pero el thread se agregó a la tabla.
            if (newpid != -1 && !status)
                space_table->Remove(newpid);

            if (status){
                // No creo que tenga sentido borrar el ejecutable.
                //delete executable;
                newThread->SetPid(newpid);
                newThread->space = space;
                newThread->Fork(ProcessInitArgs, (void*)argv);
            }
            else{
                if (newThread != nullptr)
                    delete newThread;
                if (space != nullptr)
                    delete space;
            }

            machine->WriteRegister(2, newpid);
            break;
        } 
        case SC_EXIT: {

            int ret = machine->ReadRegister(4);            

          //  delete currentThread->space;


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
            //printf("Hago JOIN, soy: %d\n", currentThread->GetPid());
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
    #ifdef USE_TLB
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &PageFaultHandler); /// Cambiar el manejador por PageFaultHandler
    #else
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &DefaultHandler); /// Cambiar el manejador por PageFaultHandler
    #endif
    machine->SetHandler(READ_ONLY_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION,      &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}
