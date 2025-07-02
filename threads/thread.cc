/// Routines to manage threads.
///
/// There are four main operations:
///
/// * `Fork` -- create a thread to run a procedure concurrently with the
///   caller (this is done in two steps -- first allocate the Thread object,
///   then call `Fork` on it).
/// * `Finish` -- called when the forked procedure finishes, to clean up.
/// * `Yield` -- relinquish control over the CPU to another ready thread.
/// * `Sleep` -- relinquish control over the CPU, but thread is now blocked.
///   In other words, it will not run again, until explicitly put back on the
///   ready queue.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread.hh"
#include "switch.h"
#include "system.hh"
#include "channel.hh"

#ifdef FILESYS
#include "filesys/file_system.hh"
#endif

#include <stdio.h>
#include <stdlib.h>

#include <cstring>

/// This is put at the top of the execution stack, for detecting stack
/// overflows.
const unsigned STACK_FENCEPOST = 0xDEADBEEF;


static inline bool
IsThreadStatus(ThreadStatus s)
{
    return 0 <= s && s < NUM_THREAD_STATUS;
}

/// Initialize a thread control block, so that we can then call
/// `Thread::Fork`.
///
/// * `threadName` is an arbitrary string, useful for debugging.
Thread::Thread(const char *threadName, bool isJoin, int threadPriority)
{
    name     = threadName;
    stackTop = nullptr;
    stack    = nullptr;
    status   = JUST_CREATED;
    // El pid se establece cuando se va a correr un proceso.
    pid = -1;
#ifdef USER_PROGRAM
    space    = nullptr;

    #ifdef FILESYS
    fileTableIds = new Table <procFileInfo*>;
    OpenFile* in = nullptr;
    OpenFile* out = nullptr;
    struct procFileInfo *newIn = new procFileInfo;
    struct procFileInfo *newOut = new procFileInfo;
    newIn->file = in;
    newIn->seek = 0;
    newOut->file = out;
    newOut->seek = 0;
    fileTableIds->Add(newIn);
    fileTableIds->Add(newOut);
    // La cantidad de subdirecciones es 0 al inicio.
    subDirectories = 0;
    // Está en el directorio root.
    strcpy(path[0],"root");
    // No hay subdirectorios.
    path[1] = nullptr;
    #else
    fileTableIds = new Table<OpenFile*>;
    OpenFile* in = nullptr;
    OpenFile* out = nullptr;
    fileTableIds->Add(in);
    fileTableIds->Add(out);
    #endif
#endif

    // JOIN IMPLEMENTATION
    join = isJoin;
    chName = threadName ? concat("JoinCh ", threadName) : nullptr;
    waitToChild = isJoin ? new Channel(chName) : nullptr;

    // Scheduler Implementation
    ASSERT( -1 < threadPriority && threadPriority < 10);
    priority = threadPriority;
}

/// De-allocate a thread.
///
/// NOTE: the current thread *cannot* delete itself directly, since it is
/// still running on the stack that we need to delete.
///
/// NOTE: if this is the main thread, we cannot delete the stack because we
/// did not allocate it -- we got it automatically as part of starting up
/// Nachos.
Thread::~Thread()
{
    DEBUG('t', "Deleting thread \"%s\"\n", name);

    delete waitToChild;
    free(chName);

    #ifdef USER_PROGRAM
    delete space;
    delete fileTableIds;
    #endif

    ASSERT(this != currentThread);
    if (stack != nullptr && name != nullptr && strcmp("main", name)) {
        SystemDep::DeallocBoundedArray((char *) stack,
                                       STACK_SIZE * sizeof *stack);
    }
}

/// Invoke `(*func)(arg)`, allowing caller and callee to execute
/// concurrently.
///
/// NOTE: although our definition allows only a single integer argument to be
/// passed to the procedure, it is possible to pass multiple arguments by
/// by making them fields of a structure, and passing a pointer to the
/// structure as "arg".
///
/// Implemented as the following steps:
/// 1. Allocate a stack.
/// 2. Initialize the stack so that a call to SWITCH will cause it to run the
///    procedure.
/// 3. Put the thread on the ready queue.
///
/// * `func` is the procedure to run concurrently.
/// * `arg` is a single argument to be passed to the procedure.
void
Thread::Fork(VoidFunctionPtr func, void *arg)
{
    ASSERT(func != nullptr);

    DEBUG('t', "Forking thread \"%s\" with func = %p, arg = %p\n",
          name, func, arg);

    StackAllocate(func, arg);

    IntStatus oldLevel = interrupt->SetLevel(INT_OFF);
    scheduler->ReadyToRun(this);  // `ReadyToRun` assumes that interrupts
                                  // are disabled!
    interrupt->SetLevel(oldLevel);
}

/// Check a thread's stack to see if it has overrun the space that has been
/// allocated for it.  If we had a smarter compiler, we would not need to
/// worry about this, but we do not.
///
/// NOTE: Nachos will not catch all stack overflow conditions.  In other
/// words, your program may still crash because of an overflow.
///
/// If you get bizarre results (such as seg faults where there is no code)
/// then you *may* need to increase the stack size.  You can avoid stack
/// overflows by not putting large data structures on the stack.  Do not do
/// this:
///         void foo() { int bigArray[10000]; ... }
void
Thread::CheckOverflow() const
{
    if (stack != nullptr) {
        ASSERT(*stack == STACK_FENCEPOST);
    }
}

void
Thread::SetStatus(ThreadStatus st)
{
    ASSERT(IsThreadStatus(st));
    status = st;
}

const char *
Thread::GetName() const
{
    return name;
}


void
Thread::Print() const
{
    printf("%s, ", name);
}

/// Called by `ThreadRoot` when a thread is done executing the forked
/// procedure.
///
/// NOTE: we do not immediately de-allocate the thread data structure or the
/// execution stack, because we are still running in the thread and we are
/// still on the stack!  Instead, we set `threadToBeDestroyed`, so that
/// `Scheduler::Run` will call the destructor, once we are running in the
/// context of a different thread.
///
/// NOTE: we disable interrupts, so that we do not get a time slice between
/// setting `threadToBeDestroyed`, and going to sleep.
void
Thread::Finish(int returnStatus)
{

    interrupt->SetLevel(INT_OFF);
    ASSERT(this == currentThread);

    DEBUG('t', "Finishing thread \"%s\"\n", GetName());
    
    // Borro mi lugar en el coremap.
    #ifdef SWAP
    for (unsigned i = 0; i < core_map->GetSize(); i++){
        if (core_map->GetPid(i) == (unsigned int)pid)
            core_map->Clear(i);
    }
    #endif 
    
    //JOIN IMPLEMENTATION
    if (join) {
        DEBUG('t', "Signal to father thread to continue with Join\n");
        waitToChild->Send(returnStatus); 
    }

    threadToBeDestroyed = currentThread;

    Sleep();  // Invokes `SWITCH`.
    // Not reached.
}

/// Relinquish the CPU if any other thread is ready to run.
///
/// If so, put the thread on the end of the ready list, so that it will
/// eventually be re-scheduled.
///
/// NOTE: returns immediately if no other thread on the ready queue.
/// Otherwise returns when the thread eventually works its way to the front
/// of the ready list and gets re-scheduled.
///
/// NOTE: we disable interrupts, so that looking at the thread on the front
/// of the ready list, and switching to it, can be done atomically.  On
/// return, we re-set the interrupt level to its original state, in case we
/// are called with interrupts disabled.
///
/// Similar to `Thread::Sleep`, but a little different.
void
Thread::Yield()
{
    IntStatus oldLevel = interrupt->SetLevel(INT_OFF);

    ASSERT(this == currentThread);

    DEBUG('t', "Yielding thread \"%s\"\n", GetName());

    Thread *nextThread = scheduler->FindNextToRun();
    if (nextThread != nullptr) {
        scheduler->ReadyToRun(this);
        scheduler->Run(nextThread);
    }

    interrupt->SetLevel(oldLevel);
}

/// Relinquish the CPU, because the current thread is blocked waiting on a
/// synchronization variable (`Semaphore`, `Lock`, or `Condition`).
/// Eventually, some thread will wake this thread up, and put it back on the
/// ready queue, so that it can be re-scheduled.
///
/// NOTE: if there are no threads on the ready queue, that means we have no
/// thread to run.  `Interrupt::Idle` is called to signify that we should
/// idle the CPU until the next I/O interrupt occurs (the only thing that
/// could cause a thread to become ready to run).
///
/// NOTE: we assume interrupts are already disabled, because it is called
/// from the synchronization routines which must disable interrupts for
/// atomicity.  We need interrupts off so that there cannot be a time slice
/// between pulling the first thread off the ready list, and switching to it.
void
Thread::Sleep()
{
    ASSERT(this == currentThread);
    ASSERT(interrupt->GetLevel() == INT_OFF);

    DEBUG('t', "Sleeping thread \"%s\"\n", GetName());

    Thread *nextThread;
    status = BLOCKED;
    while ((nextThread = scheduler->FindNextToRun()) == nullptr) {
        interrupt->Idle();  // No one to run, wait for an interrupt.
    }

    scheduler->Run(nextThread);  // Returns when we have been signalled.
}

/// ThreadFinish, InterruptEnable
///
/// Dummy functions because C++ does not allow a pointer to a member
/// function.  So in order to do this, we create a dummy C function (which we
/// can pass a pointer to), that then simply calls the member function.
static void
ThreadFinish()
{
    currentThread->Finish();
}

static void
InterruptEnable()
{
    interrupt->Enable();
}

/// Allocate and initialize an execution stack.
///
/// The stack is initialized with an initial stack frame for `ThreadRoot`,
/// which:
/// 1. enables interrupts;
/// 2. calls `(*func)(arg)`;
/// 3. calls `Thread::Finish`.
///
/// * `func` is the procedure to be forked.
/// * `arg` is the parameter to be passed to the procedure.
void
Thread::StackAllocate(VoidFunctionPtr func, void *arg)
{
    ASSERT(func != nullptr);

    stack = (uintptr_t *)
              SystemDep::AllocBoundedArray(STACK_SIZE * sizeof *stack);

    // Stacks in x86 work from high addresses to low addresses.
    stackTop = stack + STACK_SIZE - 4;  // -4 to be on the safe side!

    // x86 passes the return address on the stack.  In order for `SWITCH` to
    // go to `ThreadRoot` when we switch to this thread, the return address
    // used in `SWITCH` must be the starting address of `ThreadRoot`.
    *--stackTop = (uintptr_t) ThreadRoot; // ThreadRoot hace que el hilo ejecute la funcion con la que fué llamada. Implementada en assembler. 

    *stack = STACK_FENCEPOST;

    machineState[PCState]         = (uintptr_t) ThreadRoot;
    machineState[StartupPCState]  = (uintptr_t) InterruptEnable;
    machineState[InitialPCState]  = (uintptr_t) func;
    machineState[InitialArgState] = (uintptr_t) arg;
    machineState[WhenDonePCState] = (uintptr_t) ThreadFinish;
}

#ifdef USER_PROGRAM
#include "machine/machine.hh"

/// Save the CPU state of a user program on a context switch.
///
/// Note that a user program thread has *two* sets of CPU registers -- one
/// for its state while executing user code, one for its state while
/// executing kernel code.  This routine saves the former.
void
Thread::SaveUserState()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++) {
        userRegisters[i] = machine->ReadRegister(i);
    }
}

/// Restore the CPU state of a user program on a context switch.
///
/// Note that a user program thread has *two* sets of CPU registers -- one
/// for its state while executing user code, one for its state while
/// executing kernel code.  This routine restores the former.
void
Thread::RestoreUserState()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++) {
        machine->WriteRegister(i, userRegisters[i]);
    }
}

#endif

// --------------- JOIN IMPLEMENTATION --------------------
int
Thread::Join() {

    ASSERT(join);
    int ret;
    
    //Espero la respuesta del Child de que terminó
    waitToChild->Receive(&ret);

    return ret;
}

// --------------- SCHEDULER IMPLEMENTATION --------------------
int
Thread::GetPriority() const
{
    return priority;
}

void
Thread::SetPriority(int priorityToBeSet)
{
    priority = priorityToBeSet;
}

ThreadStatus 
Thread::GetStatus() {
    return status;
}

const char *
Thread::PrintStatus()
{
    switch (GetStatus()) {
        case BLOCKED:
            return "Blocked";
        case READY:
            return "Ready";
        case RUNNING:
            return "Running";
        case JUST_CREATED:
            return "Just Created";
        case NUM_THREAD_STATUS:
            return "4";
    }
    return "";
}
// ----------------FILES--------------
#if defined(USER_PROGRAM) && defined (FILESYS)
int
Thread::AddFile(OpenFile* file, char* Filename)
{
    struct procFileInfo *newFile = new procFileInfo;
    newFile->file = file;
    newFile->seek = 0;
    newFile->name = new char[FILE_NAME_MAX_LEN];
    strcpy(newFile->name, Filename);
    
    return fileTableIds->Add(newFile);
}

OpenFile*
Thread::GetFile(int fd)
{
    struct procFileInfo *fileInfo = fileTableIds->Get(fd);
    return fileInfo == nullptr ? nullptr : fileInfo->file;
}

int 
Thread::GetFileSeek(int fd)
{
    struct procFileInfo *fileInfo = fileTableIds->Get(fd);
    ASSERT(fileInfo != nullptr);
    return fileInfo->seek;
}

int 
Thread::AddFileSeek(int fd, int q)
{
    ASSERT(q >= 0);
    struct procFileInfo *fileInfo = fileTableIds->Get(fd);
    ASSERT(fileInfo != nullptr);
    fileInfo->seek += q;
    return fileInfo->seek;
}

OpenFile*
Thread::RemoveFile(int fd)
{
    struct procFileInfo *fileInfoRem;
    fileInfoRem = fileTableIds->Remove(fd);
    return fileInfoRem == nullptr ? nullptr : fileInfoRem->file;
}

char*
Thread::GetFileName(int fd)
{
    struct procFileInfo *fileInfo;
    fileInfo = fileTableIds->Get(fd);

    return fileInfo == nullptr ? nullptr : fileInfo->name;
}

bool
Thread::ChangeDir(char* newDir)
{
    
    char* dirNames[NUM_SECTORS];
    dirNames[0] = strtok(newDir, "/");

    // El primer directorio debe ser el actual.
    if(!strcmp(dirNames[0], "root")){
        DEBUG('f', "Error: Soy thread %d y el primer directorio del path ingresado no es root, es %s\n", pid, dirNames[0]);
        return false;
    }

    unsigned subdirs = 0;
    
    // Voy metiendo los nombres.
    while(dirNames[subdirs] != NULL){
        ASSERT(subdirs < NUM_SECTORS);
        dirNames[subdirs + 1] = strtok(NULL, "/");
        subdirs++;
    }
    
    ASSERT(subdirs < MAX_DIRS);

    // Significa que quiero ir uno atrás o uno adelante.
    // Tengo que tener en cuenta 3 casos:
    // * Es el directorio ".." -> Bajo la cantidad de subdirecciones en 1.
    // * Es un directorio cualquiera -> Quiero acceder a un nivel más de profundiad.
    // Debo checkear si el camino tiene sentido. De tener sentido, lo agrego al
    // path y aumento las subdirecciones. De no tener sentido retorno falso y aviso
    // al usuario por medio de la syscall.
    // * Es el directorio "." -> No hago nada.
    if (subdirs == 1)
    {
        if (!strcmp(dirNames[0], ".."))
        {
            if (subDirectories == 0){
                DEBUG('f', "Soy thread de pid %d. Permanezco en root\n", pid);
                return true;
            }
            
            subDirectories -= 1;
            return true;
        }

        if (!strcmp(dirNames[0], ".")){
            DEBUG('f', "Soy thread de pid %d. Me quedo en el directorio actual\n", pid);
            return true;
        }
        
        // En este caso quiero cambiar de directorio a uno siguiente.
        // Tengo que checkear que el path esté bien antes de hacerlo.
        
        char* testPath[MAX_DIRS];
        for (unsigned i = 0; i < subDirectories; i++)
            strcpy(testPath[i],path[i]);
        strcpy(testPath[subDirectories],dirNames[0]);
        testPath[subDirectories + 1] = nullptr;

        /////////////DEBUG////////////////////////////
        DEBUG('f', "El testPath del thread %u es:\n");
        for (unsigned i = 0; i <= subDirectories; i++)
            DEBUG('f', "%s\n", testPath[i]);
        //////////////////////////////////////////////
        
        if (fileSystem->CheckPath(testPath, subDirectories + 1))
        {
            // El directorio tiene sentido.
            // Por lo tanto ahora debo cambiar mi directorio y,
            // si no está en la DirTable, agregarlo. Para esto
            // tomo el lock del directorio anterior (que seguro
            // está en la dirTable).

            // Cambio mi directorio.
            subDirectories += 1;
            strcpy(path[subDirectories],dirNames[0]);
            path[subDirectories + 1] = nullptr;

            // Si el directorio no está en la tabla, es el primer
            // thread que lo abre. Por lo que tiene que recuperar
            // su archivo.
            if (dirTable->CheckDirInTable(dirNames[0]) == -1){
                DEBUG('f', "Soy thread %d, recupero el archivo de directorio %s\n", pid, path[subDirectories]);
                fileSystem->AddDirFile(path, subDirectories);
            }
            
            return true;
        }
        DEBUG('f', "Error: Thread %d. El directorio %s no forma parte del padre %s.\n", pid, dirNames[0], path[subDirectories]);
        return false;
    }

    DEBUG('f', "Soy %u, es cambio de directorio completo\n", pid);

    // Ahora además de checkear todo el path, tengo que ver si cada uno 
    // de los directorios puestos está en la tabla y si alguno no está, agregarlo.
    // Empiezo de root que siempre está en la tabla.
    
    if(!fileSystem->CheckPath(dirNames, subdirs)){
        DEBUG('f',"Error: Soy %d y el path nuevo es incorrecto\n");
        return false;
    }

    // Cambio el path actual.
    for(unsigned i = 0; i < subdirs;i++)
        strcpy(path[i], dirNames[i]);
    
    path[subdirs] = nullptr;
    subDirectories = subdirs - 1;

    // Acá significa que el path es correcto. Debo ver que cada uno esté en la tabla
    // y de no estarlo agregarlo.
    for(unsigned i = 0; i <= subDirectories;i++){
        if (i == 0)
            ASSERT(dirTable->CheckDirInTable(dirNames[0]) != -1);
        else{
            if (dirTable->CheckDirInTable(dirNames[i]) == -1)
                fileSystem->AddDirFile(path, subDirectories);
        }
    }

    return true;
}

char*
Thread::GetDir()
{
    return path[subDirectories];
}


#endif
// --------------- PID----------------

void
Thread::SetPid(int new_pid)
{
    ASSERT(new_pid != -1);
    pid = new_pid; 
    return;
}

int 
Thread::GetPid()
{
    ASSERT(pid != -1);
    return pid;
}
