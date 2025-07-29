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
            
            // unsigned initialSize = machine->ReadRegister(5);
            
            // Dado que todavía el FileSistem no permite archivos extensibles, le damos
            // un tamaño inicial.
            // El tamaño máximo es 121KiB.
            // Esto es ya que se utilizan 8KiB para guardar más estructuras que 
            // los datos del arhivo en sí.
            // Lo ponemos en 0 para el ejercicio 3. 
            // Será un archivo extensible.
            unsigned initialSize = 0;
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
            
            DEBUG('f', "Opening a file\n");

            if (filenameAddr == 0){
                DEBUG('f', "Error: address to filename string is null. \n");
                status = -1;
            }

            if (!status && !ReadStringFromUser(filenameAddr, 
                                    filename, sizeof filename)){
                 DEBUG('f', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = -1;
            }
        
            if (!status && !(newFile = fileSystem->Open(filename))){
                DEBUG('f', "Cannot open file %s.\n", filename);
                status = -1; 
            }

            if (!status){
                DEBUG('f', "`Open` requested for file `%s`.\n", filename);
                #ifndef FILESYS
                status = currentThread->fileTableIds->Add(newFile);
                #else
                DEBUG('f', "Soy %d, abro el archivo %s, lo tienen abierto %d.\n", currentThread->GetPid(), filename, fileTable->GetOpen(filename));
                status = currentThread->AddFile(newFile, filename);
                #endif
            }
            machine->WriteRegister(2, status);
            break;
        } 
        
        case SC_CLOSE: {
            
            OpenFileId fid = machine->ReadRegister(4);
            OpenFile *file;

            #ifdef FILESYS
            char* filename;
            #endif

            int status = 0;
            DEBUG('f', "`Close` requested for id %u.\n", fid);

            if (fid == 0 || fid == 1){
                DEBUG('f', "Cannot close console fd id %u.\n", fid);
                status = -1;
            }
            
            if (!status && fid < 0){
                DEBUG('f', "Bad fd id %u.\n", fid);
                status = -1;
            }
            // Sacarlo de la tabla
            #ifndef FILESYS
            if (!status && !(file = currentThread->fileTableIds->Remove(fid))){
                DEBUG('f', "Cannot found fd id %u in table.\n", fid);
                status = -1;
            }
            #else
            filename = currentThread->GetFileName(fid);
            
            // El archivo no está abierto.
            if (filename == nullptr)
                status = -1;

            if (!status && !(file = currentThread->RemoveFile(fid))){
                DEBUG('f', "Cannot found fd id %u in table.\n", fid);
                status = -1;
            }
            
            if(status != -1) {
                // El archivo fué cerrado.
                // Hay que eliminarlo unicamente si el proceso es el último
                // en tenerlo abierto.
                
                fileTable->FileORLock(filename, ACQUIRE);
                
                int opens = fileTable->GetOpen(filename);
                
                if (opens > 1){
                    DEBUG('f', "Soy %d, cierro el archivo %s y no soy el último, quedan %d\n", currentThread->GetPid(), filename, opens);
                    status = fileTable->CloseOne(filename);
                    if (status > 0)
                        status = 0;
                    machine->WriteRegister(2,status);
                    fileTable->FileORLock(filename, RELEASE);
                    break;
                }

                // Soy el último proceso que tiene abierto el archivo.
                if (opens == 1) 
                {
                    DEBUG('f', "Soy el último, cerrando completamente el archivo %s\n", filename);
                    fileTable->CloseOne(filename);

                    if (fileTable->isDeleted(filename))
                    {
                        // Soy el último y el archivo tiene que ser borrado.
                        // Por lo tanto aviso al thread que estaba esperando para hacerlo.
                        DEBUG('f', "Cierro último el archivo %s y debe ser eliminado\n", filename);
                        fileTable->FileRemoveCondition(filename, SIGNAL);
                    }
                    

                    DEBUG('f', "Deleteo %s\n", filename);
                    fileTable->SetClosed(filename, true);
                    
                    delete file;

                    machine->WriteRegister(2,status);
                    fileTable->FileORLock(filename, RELEASE);
                    break;
                }
                
                // Si llegó acá es porque opens no tiene un valor válido.
                status = -1;
            }
            #endif
            
            DEBUG('f', "NO TENGO QUE ESTAR ACA\n");
            // Sacarlo de memoria 
            if (!status)
                delete file;

            machine->WriteRegister(2, status);
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
            
            #ifndef FILESYS
            DEBUG('f', "Reading file not FILESYS\n");
            if(!status && id != 0 && !(file = currentThread->fileTableIds->Get(id))) {
                DEBUG('e', "Error: File not found. \n");
                status = -1;
            }
            #else
            if(!status && id != 0 && !(file = currentThread->GetFile(id))) {
                DEBUG('e', "Error: File not found. \n");
                status = -1;
            }
            
            char* filename = currentThread->GetFileName(id);

            if (filename == nullptr)
                status = -1;
            
            DEBUG('f', "Reading file %s\n", filename);

            // Si está todo bien y no estoy escribiendo a consola.
            if (status != -1 && id != 0) {
                
                // Aumento la cantidad de lectores
                // y de paso checkeo que no se esté escribiendo el archivo.
                fileTable->FileRdWrLock(filename, ACQUIRE);
                fileTable->AddReader(filename);
                fileTable->FileRdWrLock(filename, RELEASE);
                
                status = file->ReadAt(bufferTransfer, bytesToRead, currentThread->GetFileSeek(id));
                
                DEBUG('f', "Leí %s con una cantidad de bytes de %d en el archivo %s. Offset actual: %d\n", bufferTransfer, status, filename, currentThread->GetFileSeek(id));
                // PARA TESTEAR YA QUE NO HAY READ_AT
                //status = file->ReadAt(bufferTransfer, bytesToRead, 0);
                
                if (status != 0){ // NO estoy en EOF
                    WriteBufferToUser(bufferTransfer, bufferToWrite, status);
                    currentThread->AddFileSeek(id, bytesToRead); 
                }

                fileTable->FileRdWrLock(filename, ACQUIRE);
                fileTable->RemoveReader(filename);
                if (fileTable->GetReaders(filename) == 0 && fileTable->GetWriter(filename))
                    fileTable->FileWriterCondition(filename, SIGNAL);

                fileTable->FileRdWrLock(filename, RELEASE);
                
                machine->WriteRegister(2,status);
                break;
            }
            #endif
            
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
            
            DEBUG('f', "Writing file of id: %d\n", id);
            
            if (id == 0){
                DEBUG('f', "Error: File Descriptor Stdin");
                status = -1;
            }
            
            if (!status && bufferToRead == 0){
                DEBUG('f', "Error: Buffer to read is null. \n");
                status = -1;
            }
            
            if (!status && bytesToWrite <= 0) {
                DEBUG('f', "Error: Bytes to write is non positive. \n");
                status = -1;
            }
            
            //Buscar id
            #ifndef FILESYS
            if(!status && id != 1 && !(file = currentThread->fileTableIds->Get(id))) {
                DEBUG('f', "Error: File not found. \n");
                status = -1;
            }
            #else
            if(!status && id != 1 && !(file = currentThread->GetFile(id))) {
                DEBUG('f', "Error: File not found. \n");
                status = -1;
            }
            
            char* filename = currentThread->GetFileName(id);
            
            if (filename == nullptr)
            status = -1;
            
            DEBUG('f', "Writing file %s\n", filename);
            ASSERT(file == fileTable->GetFile(filename));
            
            // Si está todo bien y no estoy escribiendo a consola.
            if (status != -1 && id != 1) {
                // Tengo el Lock, tengo que checkear que no haya ecritores.
                fileTable->FileWrLock(filename, ACQUIRE);
                fileTable->FileRdWrLock(filename, ACQUIRE);
                fileTable->SetWriter(filename, true);

                int readers = fileTable->GetReaders(filename);
               
                DEBUG('f', "Por escribir en %s, soy %d.\n", filename, currentThread->GetPid());
                if (readers > 0)
                    fileTable->FileWriterCondition(filename, WAIT);

                // Cuando salga de acá, tiene el lock tomado y no hay lectores.
                // Por lo tanto, escribo.
                ReadBufferFromUser(bufferToRead, bufferTransfer, bytesToWrite);
                status = file->WriteAt(bufferTransfer, bytesToWrite, currentThread->GetFileSeek(id));
                currentThread->AddFileSeek(id, bytesToWrite); 
                DEBUG('f', "Escribí %s con una cantidad de bytes de %d en file %s. Offset actual: %d\n", bufferTransfer, status, filename, currentThread->GetFileSeek(id));
                
                machine->WriteRegister(2,status);
                fileTable->FileRdWrLock(filename, RELEASE);
                fileTable->FileWrLock(filename, RELEASE);
                break;
            }
            #endif
            
            DEBUG('f', "Con FS activado, si leo esto es porque o status es -1 o estoy escribiendo a STDOUT. Status: %d, fd: %d\n", status, id);
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
                DEBUG('f', "Error: address to filename string is null. \n");
                status = false;
            }

            if (!ReadStringFromUser(filenameAddr, 
                                    filename, sizeof filename) || !status){
                DEBUG('f', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = false;
            }

            if (!(executable = fileSystem->Open(filename)) || !status) {
                DEBUG('f', "Error: Unable to open file %s\n", filename);
                status = false;
            }

            if (!(newThread = new Thread(nullptr,join ? true : false)) || !status){
                DEBUG('f', "Error: Unable to create a thread %s\n", filename);
                status = false; 
            }
            
            int newpid = -1;
            if (status)
                newpid = space_table->Add(newThread);
            
            if (newpid == -1 && status)
            {
                DEBUG('f', "Error: No se puede agregar el Thread a la space_table\n");
                status = false;
            }

            if (!(space = new AddressSpace(executable, newpid)) || !status){
                DEBUG('f', "Error: Unable to create the address space \n");
                status = false;
            }

            // Caso en que falló la creación del AddressSpace pero el thread se agregó a la tabla.
            if (newpid != -1 && !status)
                space_table->Remove(newpid);

            if (status){
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
            OpenFile *executable;
            Thread* newThread;            
            char** argv;
            int newpid = -1;
            AddressSpace *space; 
            char filename[FILE_NAME_MAX_LEN + 1];

            if (filenameAddr == 0){
                DEBUG('f', "Error: address to filename string is null. \n");
                machine->WriteRegister(2, newpid);
                break;
            }

            if (argsVector == 0){
                DEBUG('f', "Error: address to argsVector is null. \n");
                machine->WriteRegister(2, newpid);
                break;
            }

            if (!ReadStringFromUser(filenameAddr, 
                                    filename, sizeof filename)){
                 DEBUG('f', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, newpid);
                break;
            }

            if (!(argv = SaveArgs(argsVector))){
                 DEBUG('f', "Error: Unable to get User Args Vectors.\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, newpid);
                break;
            }

            if (!(executable = fileSystem->Open(filename))) {
                DEBUG('f', "Error: Unable to open file %s\n", filename);
                machine->WriteRegister(2, newpid);
                break;
            }

            if (!(newThread = new Thread(nullptr, join ? true : false))) {
                DEBUG('f', "Error: Unable to create a thread %s\n", filename);
                machine->WriteRegister(2, newpid);
                break;
            }

            if ((newpid = space_table->Add(newThread)) == -1){
                DEBUG('f', "Error: No se puede agregar el Thread a la space_table\n");
                delete newThread;
                machine->WriteRegister(2, newpid);
                break;
            }

            if (!(space = new AddressSpace(executable, newpid))){
                DEBUG('f', "Error: Unable to create the address space \n");
                space_table->Remove(newpid);
                delete newThread;
                machine->WriteRegister(2, newpid);
                break;
            }

            newThread->SetPid(newpid);
            newThread->space = space;
            newThread->Fork(ProcessInitArgs, (void*)argv);

            machine->WriteRegister(2, newpid);
            break;
        } 
        case SC_EXIT: {

            int ret = machine->ReadRegister(4);            

            if (space_table->Get(0) == currentThread) // Main thread Exit
                interrupt->Halt();

            currentThread->Finish(ret); 
            break;
        }
        case SC_JOIN: {
            SpaceId childId = machine->ReadRegister(4); 
            int status = 0;
            Thread *child;

            DEBUG('f', "Hago join al id: %d\n", childId);
            if (!(child = space_table->Get(childId))){
                DEBUG('f', "Error: Unable to get the childId.\n");
                status = -1;
            }
            if (!status){
                DEBUG('f', "Voy a sacar childId de la space-table\n");
                space_table->Remove(childId);
                DEBUG('f', "Llamo a Join\n");
                status = child->Join();
            }
            
            machine->WriteRegister(2, status);
            DEBUG('f', "Hago JOIN, soy: %d\n", currentThread->GetPid());
            break;
        }
        #ifdef FILESYS
        case SC_MKDIR: {
            int dirNameAddr = machine->ReadRegister(4);
            bool status = true;
            char dirname[FILE_NAME_MAX_LEN + 1];
            
            DEBUG('f', "Creating a directory\n");

            if (!ReadStringFromUser(dirNameAddr, 
                                    dirname, sizeof dirname)){
                 DEBUG('f', "Error: dirname string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = false;
            }
            
            if(fileSystem->MkDir(dirname,0))
                status = 0;
            else 
                status = -1;

            machine->WriteRegister(2,status);
            DEBUG('f',"Creé el directorio %s, soy %d.\n", dirname, currentThread->GetPid());
            break;
        }
        
        case SC_RMDIR: {
            
            int dirNameAddr = machine->ReadRegister(4);
            bool status = true;
            char dirname[FILE_NAME_MAX_LEN + 1];
            
            DEBUG('f', "Removing a directory\n");

            if (!ReadStringFromUser(dirNameAddr, 
                                    dirname, sizeof dirname)){
                 DEBUG('f', "Error: dirname string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = false;
            }
            
            if(fileSystem->RemoveDir(dirname))
                status = 0;
            else 
                status = -1;

            machine->WriteRegister(2,status);
            DEBUG('f',"Eliminé el directorio %s, soy %d.\n", dirname, currentThread->GetPid());
            break;

        }
        case SC_CDIR: {

            int dirNameAddr = machine->ReadRegister(4);
            bool status = true;
            char dirname[FILE_NAME_MAX_LEN + 1];
            
            DEBUG('f', "Changing directory\n");

            if (!ReadStringFromUser(dirNameAddr, 
                                    dirname, sizeof dirname)){
                 DEBUG('f', "Error: dirname string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = false;
            }
            
            if(currentThread->ChangeDir(dirname))
                status = 0;
            else 
                status = -1;

            machine->WriteRegister(2,status);
            DEBUG('f',"Cambié al directorio %s, soy %d.\n", dirname, currentThread->GetPid());
            break;
        }
        case SC_LSDIR: {
            
            int dirNameAddr = machine->ReadRegister(4);
            bool status = true;
            char dirname[FILE_NAME_MAX_LEN + 1];
            
            DEBUG('f', "Listing directory\n");

            if (!ReadStringFromUser(dirNameAddr, 
                                    dirname, sizeof dirname)){
                 DEBUG('f', "Error: dirname string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                status = false;
            }
            
            if(fileSystem->Ls(dirname))
                status = 0;
            else 
                status = -1;

            machine->WriteRegister(2,status);
            DEBUG('f',"Listé el directorio %s, soy %d.\n", dirname, currentThread->GetPid());

            break;
        }
        #endif
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
