/// Routines to manage the overall operation of the file system.  Implements
/// routines to map from textual file names to files.
///
/// Each file in the file system has:
/// * a file header, stored in a sector on disk (the size of the file header
///   data structure is arranged to be precisely the size of 1 disk sector);
/// * a number of data blocks;
/// * an entry in the file system directory.
///
/// The file system consists of several data structures:
/// * A bitmap of free disk sectors (cf. `bitmap.h`).
/// * A directory of file names and file headers.
///
/// Both the bitmap and the directory are represented as normal files.  Their
/// file headers are located in specific sectors (sector 0 and sector 1), so
/// that the file system can find them on bootup.
///
/// The file system assumes that the bitmap and directory files are kept
/// “open” continuously while Nachos is running.
///
/// For those operations (such as `Create`, `Remove`) that modify the
/// directory and/or bitmap, if the operation succeeds, the changes are
/// written immediately back to disk (the two files are kept open during all
/// this time).  If the operation fails, and we have modified part of the
/// directory and/or bitmap, we simply discard the changed version, without
/// writing it back to disk.
///
/// Our implementation at this point has the following restrictions:
///
/// * there is no synchronization for concurrent accesses;
/// * files have a fixed size, set when the file is created;
/// * files cannot be bigger than about 3KB in size;
/// * there is no hierarchical directory structure, and only a limited number
///   of files can be added to the system;
/// * there is no attempt to make the system robust to failures (if Nachos
///   exits in the middle of an operation that modifies the file system, it
///   may corrupt the disk).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "directory.hh"
#include "file_header.hh"

#include "filesys/raw_file_header.hh"
#include "threads/system.hh"
#include <stdio.h>
#include <string.h>
#include <cstring>

#define MIN(a,b) a < b ? a : b
#define MAX_DIR_ENTRIES 50

Lock* CreateLock = new Lock("FSCreateLock");
/// Sectors containing the file headers for the bitmap of free sectors, and
/// the directory of files.  These file headers are placed in well-known
/// sectors, so that they can be located on boot-up.
static const unsigned FREE_MAP_SECTOR = 0;
static const unsigned DIRECTORY_SECTOR = 1;

/// Initialize the file system.  If `format == true`, the disk has nothing on
/// it, and we need to initialize the disk to contain an empty directory, and
/// a bitmap of free sectors (with almost but not all of the sectors marked
/// as free).
///
/// If `format == false`, we just have to open the files representing the
/// bitmap and the directory.
///
/// * `format` -- should we initialize the disk?
FileSystem::FileSystem(bool format)
{
    DEBUG('f', "Initializing the file system.\n");
    // Debemos inicializar el disco (de 0)
    if (format) {
        
        // Creamos un bitmap para ir llevando los sectores libres del disco.
        Bitmap     *freeMap = new Bitmap(NUM_SECTORS);
        
        // No creamos un directorio ya que no vamos a guardar nada.
        // En el directorio se guarda unicamente la tabla de archivos que tiene.
        // En este momento no tiene nada.
        //Directory  *dir     = new Directory(NUM_DIR_ENTRIES);
        FileHeader *mapH    = new FileHeader;
        FileHeader *dirH    = new FileHeader;

        DEBUG('f', "Formatting the file system.\n");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        // Los FileHeaders del bitmap y del directorio principal están en los
        // sectores 0 y 1 respectivamente.
        // Siempre habrá un directorio root.
        // Los archivos/directorios se crean dentro de este.
        freeMap->Mark(FREE_MAP_SECTOR);
        freeMap->Mark(DIRECTORY_SECTOR);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!
        
        DEBUG('f', "Hago espacio para los datos del bitmap\n");
        ASSERT(mapH->Allocate(freeMap, FREE_MAP_FILE_SIZE));
        DEBUG('f', "Hago espacio para los datos del directorio\n");
        ASSERT(dirH->Allocate(freeMap, DIRECTORY_FILE_SIZE));

        // Flush the bitmap and directory `FileHeader`s back to disk.
        // We need to do this before we can `Open` the file, since open reads
        // the file header off of disk (and currently the disk has garbage on
        // it!).
        
        // Esto permite eliminar mapH y dirH ya que la información importante
        // (RawFileHeader) la mando a disco. Una unidad estática.
        DEBUG('f', "Writing headers back to disk.\n");
        mapH->WriteBack(FREE_MAP_SECTOR);
        dirH->WriteBack(DIRECTORY_SECTOR);

        // OK to open the bitmap and directory files now.
        // The file system operations assume these two files are left open
        // while Nachos is running.

        OpenFile* freeMapFile   = new OpenFile(FREE_MAP_SECTOR);
        OpenFile* directoryFile = new OpenFile(DIRECTORY_SECTOR);

        // Añadimos el freeMap a la fileTable
        fileTable->Add(freeMapFile, "freeMap");

        // Añadimos el directorio a la dirTable.
        dirTable->Add(directoryFile, "root", nullptr);
        
        // Once we have the files “open”, we can write the initial version of
        // each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG('f', "Writing bitmap and directory back to disk.\n");
        freeMap->WriteBack(fileTable->GetFile("freeMap"));     // flush changes to disk
        
        //dir->WriteBack(directoryFile);

        if (debug.IsEnabled('f')) {
            freeMap->Print();
            //dir->Print();

            delete freeMap;
            //delete dir;
            delete mapH;
            delete dirH;
        }
    } else {
        // If we are not formatting the disk, just open the files
        // representing the bitmap and directory; these are left open while
        // Nachos is running.
        OpenFile* freeMapFile   = new OpenFile(FREE_MAP_SECTOR);
        OpenFile* directoryFile = new OpenFile(DIRECTORY_SECTOR);
        
        // Añadimos el freeMap a la fileTable
        fileTable->Add(freeMapFile, "freeMap");
        
       unsigned dirEntries = 0;
        
       if (directoryFile->GetFileHeader()->GetRaw()->numSectors > 0){
            Directory* dir = new Directory(MAX_DIR_ENTRIES);
            dir->FetchFrom(directoryFile);
            for(; dir->GetRaw()->table[dirEntries].inUse;dirEntries++);
       }

        // Añadimos el directorio a la dirTable.
        dirTable->Add(directoryFile, "root", nullptr);

        // Seteo el número en la dirTable para usos posteriores.
        dirTable->SetNumEntries("root", dirEntries);
        DEBUG('f', "La cantidad de dirEntries es: %u\n", dirEntries);
    }

    DEBUG('f', "End of creating FileSystem\n");
}

FileSystem::~FileSystem()
{
    delete fileTable->GetFile("freeMap");
    delete dirTable->GetDir("root");
    //fileTable->Remove("freeMap");
    // Ver en el ejercicio 4 que pasa al remover directorios.
}

bool
FileSystem::CheckPath(char** dirNames, unsigned subdirs)
{

    if (strcmp(dirNames[0],"root") != 0)
        return false;

    // Ahora la cantidad de directorios es subdirs.
    // Debo buscar que cada uno de ellos se encuentre dentro del anterior.
    Directory* subDirs[subdirs];
    int subDirSector;

    // Checkeo que el path sea correcto.
    // Es decir, checkeo que cada directorio pertenezca al anterior.
    for (unsigned i = 0; i < subdirs; i++)
    {
        //dirTable->DirLock(dirNames[i], ACQUIRE);
        subDirs[i] = new Directory(dirTable->GetNumEntries(dirNames[i]));
        subDirs[i]->FetchFrom(dirTable->GetDir(dirNames[i]));
        if(dirNames[i+1] != NULL)
        {
            // Busco el directorio donde quiero agregar el archivo.
            // Si no existe, retorno error.
            subDirSector = subDirs[i]->Find(dirNames[i+1]);
            if(subDirSector == -1){
                DEBUG('f', "Error: Directorio %s no existente\n", dirNames[i+1]);
                return false;
            }

            // Acá ya sé que el directorio pertenece.
            // Suelto el lock y elimino el actual.
            // dirTable->DirLock(dirNames[i], RELEASE);
            delete subDirs[i];
        }
    }
    return true;
}

bool
FileSystem::AddDirFile(char** path, unsigned subDirectories)
{
    // El primer directorio tiene que ser el root.
    ASSERT(!strcpy(path[0],"root"));

    // Luego solo me hace falta el penúltimo, ya que será
    // el padre del último -> El que necesito.

    dirTable->DirLock(path[subDirectories-1], ACQUIRE);
    //Esta operación se hace con los locks correspondientes tomados.
    Directory* dir = new Directory(dirTable->GetNumEntries(path[subDirectories-1]));
    dir->FetchFrom(dirTable->GetDir(path[subDirectories-1]));
    
    
    int sub_sector = dir->Find(path[subDirectories]);
    // No tendría sentido que de -1 ya que antes de entrar a esta función
    // ya corroboré que tenga sentido la cadena de directorios.
    ASSERT(sub_sector != -1);
    
    OpenFile* entrySearched = new OpenFile(sub_sector);

    Directory* subDir = new Directory(MAX_DIR_ENTRIES);
    subDir->FetchFrom(entrySearched);
    
    unsigned numSubDirEntries = 0;
    if(entrySearched->GetFileHeader()->GetRaw()->numSectors != 0)
        for(;subDir->GetRaw()->table[numSubDirEntries].inUse;numSubDirEntries++);
    dirTable->Add(entrySearched, path[subDirectories], path[subDirectories-1]);
    dirTable->SetNumEntries(path[subDirectories], numSubDirEntries);
    dirTable->DirLock(path[subDirectories-1], RELEASE);
    delete subDir;
    delete dir;
    

    return true;
}

/// Create a file in the Nachos file system (similar to UNIX `create`).
/// Since we cannot increase the size of files dynamically, we have to give
/// `Create` the initial size of the file.
///
/// The steps to create a file are:
/// 1. Make sure the file does not already exist.
/// 2. Allocate a sector for the file header.
/// 3. Allocate space on disk for the data blocks for the file.
/// 4. Add the name to the directory.
/// 5. Store the new file header on disk.
/// 6. Flush the changes to the bitmap and the directory back to disk.
///
/// Return true if everything goes ok, otherwise, return false.
///
/// Create fails if:
/// * file is already in directory;
/// * no free space for file header;
/// * no free entry for file in directory;
/// * no free space for data blocks for the file.
///
/// Note that this implementation assumes there is no concurrent access to
/// the file system!
///
/// * `name` is the name of file to be created.
/// * `initialSize` is the size of file to be created.
bool
FileSystem::Create(const char *name, unsigned initialSize)
{
    // Se podría usar esta misma Create para crear directorios (como MKDIR)
    // ya que tienen la misma sintáxis y únicamente cambia la semántica que se le da.
    // Ver bien eso.

    //CreateLock->Acquire();

    // No hace falta hacer una copia del nombre ya que 
    // para lo único que se usa es para agregarlo a la dirTable
    // y esta en su función Add ya realiza la copia.
    ASSERT(name != nullptr);
    unsigned nameLen = strlen(name);
    ASSERT(nameLen < FILE_NAME_MAX_LEN);
    ASSERT(initialSize < MAX_FILE_SIZE);
    

    // Ahora tomamos el lock del directorio sobre el cual vamos a trabajar.
    // En este caso es root.
    // Podemos llevar una tabla con el path de trabajo del hilo actual
    // y quedarnos con el último.
    // Siempre verificando que ese path tiene sentido.
    char* actDir = currentThread->GetDir();
    ASSERT(actDir != nullptr);
    
    DEBUG('f', "Voy a crear el archivo %s en el directorio %s\n.", name, actDir);
    dirTable->DirLock(actDir, ACQUIRE);
    Directory *dir = new Directory(dirTable->GetNumEntries(actDir));
    dir->FetchFrom(dirTable->GetDir(actDir));
    
    bool success;

    if (dir->Find(name) != -1) {
        DEBUG('f', "El archivo %s ya está en el directorio\n", name);
        success = false;  // File is already in directory.
    } else {
        
        Bitmap *freeMap = new Bitmap(NUM_SECTORS);
        OpenFile* freeMapFile = fileTable->GetFile("freeMap");
        freeMap->FetchFrom(freeMapFile);
        int sector = freeMap->Find();
          // Find a sector to hold the file header.
        if (sector == -1) {
            DEBUG('f', "Error: no hay lugar para el header del archivo %s\n", name);
            success = false;  // No free block for file header.
        } else if (!dir->Add(name, sector)) {
            DEBUG('f', "Error: no hay espacio en directorio para archivo %s\n", name);
            success = false;  // No space in directory.
        } else {
            FileHeader *h = new FileHeader;
            success = h->Allocate(freeMap, initialSize);
              // Fails if no space on disk for data.
            if (success) {
                DEBUG('f', "Creación del archivo %s exitosa, mandando a disco todo\n", name);
                // Escribo el bitmap así se actualiza el sector que le dí al archivo.
                freeMap->WriteBack(freeMapFile);

                // Everything worked, flush all changes back to disk.
                DEBUG('f', "Mando a disco el header del archivo %s\n", name);
                h->WriteBack(sector);
                DEBUG('f', "Mando a disco el directorio que contiene el archivo\n");
                //dir->FetchFrom(dirTable->GetDir("root"));
                dir->WriteBack(dirTable->GetDir("root"));
                DEBUG('f', "Mando a disco el Bitmap\n");
                // Lo traigo de nuevo para ver si hubo algún cambio.
                freeMap->FetchFrom(freeMapFile);
                freeMap->WriteBack(freeMapFile);
                DEBUG('f', "Ahora quiero imprimir el Bitmap\n");
                freeMap->Print();
               // DEBUG('f',"Escribo por las dudas:\n");
               // dir->WriteBack(dirTable->GetDir("root"));
               // DEBUG('f', "A ver que tal quedó:\n");
               // dir->FetchFrom(dirTable->GetDir("root"));
            }
            else
                DEBUG('f', "Error: No hay espacio en el disco para los datos del archivo %s\n", name);

            delete h;
        }
        delete freeMap;
    }
    delete dir;
    //CreateLock->Release();
    if (success){
        DEBUG('f', "Archivo %s creado correctamente\n", name);
        dirTable->SetNumEntries("root", dirTable->GetNumEntries("root") + 1);
       // DEBUG('f', "Miro el directorio antes de salir:\n");
       // Directory* testDir = new Directory(dirTable->GetNumEntries("root"));
       // testDir->FetchFrom(dirTable->GetDir("root"));
       // delete testDir;
    }
    else
        DEBUG('f', "Archivo %s no pudo ser creado\n", name);
    
    dirTable->DirLock(actDir, RELEASE);
    return success;
}

/// Open a file for reading and writing.
///
/// To open a file:
/// 1. Find the location of the file's header, using the directory.
/// 2. Bring the header into memory.
///
/// * `name` is the text name of the file to be opened.
OpenFile *
FileSystem::Open(const char *name)
{
    ASSERT(name != nullptr);
    char* actDir = currentThread->GetDir();
    Directory *dir = new Directory(dirTable->GetNumEntries(actDir));
    OpenFile  *openFile = nullptr;

    DEBUG('f', "Opening file %s\n", name);
    dirTable->DirLock(actDir, ACQUIRE);
    dir->FetchFrom(dirTable->GetDir(actDir));
    int sector = dir->Find(name);
    dirTable->DirLock(actDir, RELEASE);
    if (sector >= 0) {
        
        // Primero debo checkear que el archivo no esté en la fileTable.
        if (fileTable->CheckFileInTable(name) != -1){
            DEBUG('f', "Archivo nuevo %s, en la FileTable\n", name);
            fileTable->FileORLock(name, ACQUIRE);

            // El archivo ha sido eliminado, no puede abrirse.
            if (fileTable->isDeleted(name)){
                DEBUG('f', "El archivo %s ha sido eliminado y no puede abrirse\n", name);
                delete dir;
                fileTable->FileORLock(name, RELEASE);
                return nullptr;
            }
            
            // Si el archivo fué cerrado. Debo crearlo de nuevo.
            if (fileTable->GetClosed(name)){
                openFile = new OpenFile(sector);
            }
            else
                openFile = fileTable->GetFile(name);

            // Lo agrego a la FileTable.
            // Si no fué cerrado simplemente aumenta en 1 la cantidad de abiertos.
            // Si fué cerrado, reemplaza el openFile por este nuevo.
            // Acá no hace falta hacer una copia del nombre ya que se hace internamente en Add.
            fileTable->Add(openFile, name);
            
            // Lo seteo como abierto.
            fileTable->SetClosed(name, false);
            
            delete dir;
            fileTable->FileORLock(name, RELEASE);
            return openFile;
        }
        DEBUG('f', "Archivo nuevo %s, no en la FileTable\n", name);        
        // En este caso el archivo está siendo abierto por primera vez
        // por lo que no es necesario tomar medidas de concurrencia.
        openFile = new OpenFile(sector);  // `name` was found in directory.
        fileTable->Add(openFile, name); 
    }
    else
        DEBUG('f', "Archivo %s no encontrado, no se puede abrir\n", name);

    delete dir;
   // DEBUG('f', "Testeando el directorio al abrir el archivo %s\n", name);
   // Directory *test = new Directory(dirTable->GetNumEntries("root"));
   // test->FetchFrom(dirTable->GetDir("root"));
    return openFile;  // Return null if not found.
}

/// Delete a file from the file system.
///
/// This requires:
/// 1. Remove it from the directory.
/// 2. Delete the space for its header.
/// 3. Delete the space for its data blocks.
/// 4. Write changes to directory, bitmap back to disk.
///
/// Return true if the file was deleted, false if the file was not in the
/// file system.
///
/// * `name` is the text name of the file to be removed.
bool
FileSystem::Remove(const char *name)
{
    // Acá si habría que hacer otra cosa en caso de eliminar directorios.
    // Habría que poner en 0 el used de todos los archivos que contiene.
    // Ver eso.

    ASSERT(name != nullptr);
    
    DEBUG('f', "El archivo %s va a ser removido\n", name);
    
    char* actDir = currentThread->GetDir();
    ASSERT(actDir != nullptr);

    dirTable->DirLock(actDir, ACQUIRE);
    Directory *dir = new Directory(dirTable->GetNumEntries(actDir));
    OpenFile* directoryFile = dirTable->GetDir(actDir);
    dir->FetchFrom(dirTable->GetDir(actDir));
    int sector = dir->Find(name);
    if (sector == -1) {
       dirTable->DirLock(actDir, RELEASE);
       delete dir;
       return false;  // file not found
    }
    
    // Encontró el archivo.

    // Si el archivo no fué abierto por nadie se puede eliminar directamente.
    if (fileTable->CheckFileInTable(name) != -1){
        // El archivo ha sido abierto, entonces está en la tabla.
        
        // Adquirimos su Lock.
        fileTable->FileORLock(name, ACQUIRE);

        // Primero checkeamos que no esté eliminado.    
        if (fileTable->isDeleted(name)){
            fileTable->FileORLock(name, RELEASE);
            return false;
        }

        // El archivo no ha sido eliminado.

        // Lo marco como eliminado.
        ASSERT(fileTable->Delete(name) != -1);
        
        // Si soy el único proceso que lo mantenía abierto
        // no espero nada, suelto el lock y lo borro.
        if (fileTable->GetOpen(name) > 1){
            dirTable->DirLock(actDir, RELEASE);
            // Debo esperar a que todos
            // los hilos que mantienen el archivo abierto lo
            // cierren para poder reclamar los sectores.
            fileTable->FileRemoveCondition(name, WAIT);
            dirTable->DirLock(actDir, ACQUIRE);
        }

        // Para este punto puedo soltar el Lock ya que no permito
        // más aberturas del archivo.
        fileTable->FileORLock(name, RELEASE);
    
        // Elimino el archivo de la fileTable.
        fileTable->Remove(name);
    }
    
    // Para este punto el archivo se puede eliminar de manera segura.
    FileHeader *fileH = new FileHeader;
    fileH->FetchFrom(sector);
    
    Bitmap *freeMap = new Bitmap(NUM_SECTORS);
    OpenFile* freeMapFile = fileTable->GetFile("freeMap"); 
    freeMap->FetchFrom(freeMapFile);

    fileH->Deallocate(freeMap);  // Remove data blocks.
    freeMap->Clear(sector);      // Remove header block.
    dir->Remove(name);

    freeMap->WriteBack(freeMapFile);  // Flush to disk.
    dir->WriteBack(directoryFile);    // Flush to disk.
    
    // No hace falta decrementar el número de dirEntries ya que
    // simplemente se marca como no en uso.
    dirTable->DirLock(actDir, RELEASE);
    delete fileH;
    delete dir;
    delete freeMap;
    return true;
}

// Para eliminar un directorio se debe estar
// dentro del directorio que lo contiene.
// Para evitar esto se puede pasar directamente un char**
// poniendo todos los directorios.
bool
FileSystem::RemoveDir(char *path)
{
    // Voy a eliminar un directorio.
    // Los checkeos que sea válido y demás están en la función de threads.
    ASSERT(path != nullptr);

    char* dirNames[NUM_SECTORS];
    dirNames[0] = strtok(path, "/");

    unsigned subdirs = 0;
    
    // Voy metiendo los nombres.
    while(dirNames[subdirs] != NULL){
        ASSERT(subdirs < NUM_SECTORS);
        dirNames[subdirs + 1] = strtok(NULL, "/");
        subdirs++;
    }
    
    ASSERT(subdirs > 0 && subdirs < MAX_DIRS);
    char* actDir;
    char* name;

    if (subdirs == 1){
        if (!strcmp(dirNames[0],"root")){
            DEBUG('f', "Error: El directorio root no puede ser eliminado.\n");
            return false;
        }
        // Este caso es el más fácil. 
        // Ya sabemos que todo el directorio del thread tiene sentido y
        // por eso no hay que checkear nada más que el directorio
        // ingresado esté dentro del actual. Eso ya lo hace la función.

        name = dirNames[0];
        actDir = currentThread->GetDir();
        ASSERT(actDir != nullptr);
        

    }
    else // Acá debo checkear todo el path. 
    {
        if(!CheckPath(dirNames, subdirs))
        {
            DEBUG('f', "Error al eliminar el directorio. Path ingresado incorrecto.\n");
            return false;
        }
        name = dirNames[subdirs-1];
        actDir = dirNames[subdirs-2];
        ASSERT(name != nullptr);
        ASSERT(actDir != nullptr);

        // Una vez que el path es válido, por las dudas, checkeo que todo 
        // el path esté en la dirTable.
        for (unsigned i = 0; i < subdirs; i++){
            if(dirTable->CheckDirInTable(dirNames[i]) == -1)
                ASSERT(AddDirFile(dirNames, i));
        }

    }
    
    // Acá estoy seguro que está todo en la dirTabe.
    DEBUG('f', "El directorio %s va a ser removido\n", name);
    

    dirTable->DirLock(actDir, ACQUIRE);
    Directory *dir = new Directory(dirTable->GetNumEntries(actDir));
    OpenFile* directoryFile = dirTable->GetDir(actDir);
    dir->FetchFrom(directoryFile);
    int sector = dir->Find(name);
    
    if (sector == -1) {
       dirTable->DirLock(actDir, RELEASE);
       delete dir;
       DEBUG('f', "Error: El directorio a eliminar %s no fué encontrado.\n", name);
       return false;  // file not found
    }

    // El directorio existe y está dentro del directorio actual,
    // pero no tiene una entrada en la tabla. Hay que crearla así pueden
    // ser eliminados los contenidos del mismo.
    OpenFile* delDirFile = new OpenFile(sector);
    Directory* delDir;
    if (dirTable->CheckDirInTable(name) != -1)
    {
        delDir = new Directory(dirTable->GetNumEntries(name));
    }
    else 
    {
        Directory* preDir = new Directory(MAX_DIR_ENTRIES);
        preDir->FetchFrom(delDirFile);
        unsigned entries = 0;
        if (delDirFile->GetFileHeader()->GetRaw()->numSectors != 0)
            for(;preDir->GetRaw()->table[entries].inUse;entries++);
        delete preDir;
        delDir = new Directory(entries);
        dirTable->Add(delDirFile, name, actDir);
        dirTable->SetNumEntries(name, entries);
    }

    delDir->FetchFrom(delDirFile);
    
    Bitmap *freeMap = new Bitmap(NUM_SECTORS);
    OpenFile* freeMapFile = fileTable->GetFile("freeMap"); 
    freeMap->FetchFrom(freeMapFile);
    
    // Si nadie lo mantiene abierto puedo cerrarlo directamente.
    if(dirTable->getThreadsIn(name) == 0)
    
    {
       // Debo recorrer todo el directorio.
       // Si un archivo está en la tabla, lo puedo desmarcar todo ya que 
       // tengo la seguridad que ningún thread está en este directorio
       // ni en los siguientes.
       // Eso significa que no hay nadie trabajando con el archivo actualmente.
       unsigned cantEntriestoDel = delDir->GetRaw()->tableSize;
       for (unsigned i = 0; i < cantEntriestoDel; i++){
            if(delDir->GetRaw()->table[i].inUse){    
                if(fileTable->CheckFileInTable(delDir->GetRaw()->table[i].name) != -1){
                        fileTable->Delete(delDir->GetRaw()->table[i].name);
                        fileTable->Remove(delDir->GetRaw()->table[i].name);
                    }
                FileHeader* hdr = new FileHeader;
                hdr->FetchFrom(delDir->GetRaw()->table[i].sector);
                hdr->Deallocate(freeMap);
                freeMap->Clear(delDir->GetRaw()->table[i].sector);
                delDir->Remove(delDir->GetRaw()->table[i].name);
                delete hdr;
            }
        }
    }
    else // Tengo que esperar que los threads estén fuera del directorio. 
    {
        dirTable->setToDelete(name); 
        dirTable->DirRemoveCondition(name, WAIT);

        // Una vez acá puedo eliminar todo de forma segura.

       unsigned cantEntriestoDel = delDir->GetRaw()->tableSize;
       for (unsigned i = 0; i < cantEntriestoDel; i++){
            if(delDir->GetRaw()->table[i].inUse){    
                if(fileTable->CheckFileInTable(delDir->GetRaw()->table[i].name) != -1){
                        fileTable->Delete(delDir->GetRaw()->table[i].name);
                        fileTable->Remove(delDir->GetRaw()->table[i].name);
                    }
                FileHeader* hdr = new FileHeader;
                hdr->FetchFrom(delDir->GetRaw()->table[i].sector);
                hdr->Deallocate(freeMap);
                freeMap->Clear(delDir->GetRaw()->table[i].sector);
                delDir->Remove(delDir->GetRaw()->table[i].name);
                delete hdr;
                delDir->Remove(delDir->GetRaw()->table[i].name);
            }
       } 
    }

    // Una vez que estoy acá ya eliminé todo y tengo que mandar los cambios a disco unicamente.
   
    freeMap->WriteBack(freeMapFile);
    delDir->WriteBack(delDirFile); 
    dirTable->DirLock(actDir, RELEASE);
    delete delDir;
    delete freeMap;
    return true;
}

bool
FileSystem::MkDir(const char *name, unsigned initialSize)
{
    // Se podría usar esta misma Create para crear directorios (como MKDIR)
    // ya que tienen la misma sintáxis y únicamente cambia la semántica que se le da.
    // Ver bien eso.

    //CreateLock->Acquire();
    ASSERT(name != nullptr);
    ASSERT(strlen(name) < DIR_NAME_MAX_LEN);
    ASSERT(initialSize < MAX_FILE_SIZE);
    
    // Ahora tomamos el lock del directorio sobre el cual vamos a trabajar.
    // En este caso es root.
    // Podemos llevar una tabla con el path de trabajo del hilo actual
    // y quedarnos con el último.
    // Siempre verificando que ese path tiene sentido.
    char* actDir = currentThread->GetDir();
    ASSERT(actDir != nullptr);
    
    DEBUG('f', "Voy a crear el subdirectorio %s en el directorio %s\n.", name, actDir);
    dirTable->DirLock(actDir, ACQUIRE);
    Directory *dir = new Directory(dirTable->GetNumEntries(actDir));
    dir->FetchFrom(dirTable->GetDir(actDir));
    
    bool success;

    if (dir->Find(name) != -1) {
        DEBUG('f', "El directorio %s ya está en el directorio\n", name);
        success = false;  // File is already in directory.
    } else {
        
        Bitmap *freeMap = new Bitmap(NUM_SECTORS);
        OpenFile* freeMapFile = fileTable->GetFile("freeMap");
        freeMap->FetchFrom(freeMapFile);
        int sector = freeMap->Find();
          // Find a sector to hold the file header.
        if (sector == -1) {
            DEBUG('f', "Error: no hay lugar para el header del archivo %s\n", name);
            success = false;  // No free block for file header.
        } else if (!dir->Add(name, sector)) {
            DEBUG('f', "Error: no hay espacio en directorio para archivo %s\n", name);
            success = false;  // No space in directory.
        } else {
            FileHeader *h = new FileHeader;
            success = h->Allocate(freeMap, initialSize);
              // Fails if no space on disk for data.
            if (success) {

                OpenFile* newDirFile = new OpenFile(sector);
                dirTable->Add(newDirFile, name, actDir);

                DEBUG('f', "Creación del directorio %s exitosa, mandando a disco todo\n", name);
                // Escribo el bitmap así se actualiza el sector que le dí al archivo.
                freeMap->WriteBack(freeMapFile);

                // Everything worked, flush all changes back to disk.
                DEBUG('f', "Mando a disco el header del archivo %s\n", name);
                h->WriteBack(sector);
                DEBUG('f', "Mando a disco el directorio que contiene el archivo\n");
                dir->WriteBack(dirTable->GetDir("root"));
                DEBUG('f', "Mando a disco el Bitmap\n");
                // Lo traigo de nuevo para ver si hubo algún cambio.
                freeMap->FetchFrom(freeMapFile);
                freeMap->WriteBack(freeMapFile);
                DEBUG('f', "Ahora quiero imprimir el Bitmap\n");
                freeMap->Print();
               // DEBUG('f',"Escribo por las dudas:\n");
               // dir->WriteBack(dirTable->GetDir("root"));
               // DEBUG('f', "A ver que tal quedó:\n");
               // dir->FetchFrom(dirTable->GetDir("root"));
            }
            else
                DEBUG('f', "Error: No hay espacio en el disco para los datos del archivo %s\n", name);

            delete h;
        }
        delete freeMap;
    }
    delete dir;
    //CreateLock->Release();
    if (success){
        DEBUG('f', "Directorio %s creado correctamente\n", name);
        dirTable->SetNumEntries("root", dirTable->GetNumEntries("root") + 1);
       // DEBUG('f', "Miro el directorio antes de salir:\n");
       // Directory* testDir = new Directory(dirTable->GetNumEntries("root"));
       // testDir->FetchFrom(dirTable->GetDir("root"));
       // delete testDir;
    }
    else
        DEBUG('f', "Archivo %s no pudo ser creado\n", name);
    
    dirTable->DirLock(actDir, RELEASE);
    return success;
}

// Lista todos los contenidos de un directorio dado.
bool
FileSystem::Ls(char* path)
{
    DEBUG('f', "Voy a listar los directorios.\n");
    ASSERT(path != nullptr);

    char* dirNames[NUM_SECTORS];
    dirNames[0] = strtok(path, "/");

    unsigned subdirs = 0;
    
    // Voy metiendo los nombres.
    while(dirNames[subdirs] != NULL){
        ASSERT(subdirs < NUM_SECTORS);
        dirNames[subdirs + 1] = strtok(NULL, "/");
        subdirs++;
    }
    
    ASSERT(subdirs > 0 && subdirs < MAX_DIRS);
    char* name;

    if (subdirs == 1){
        if (!strcmp(dirNames[0],"root")){
            DEBUG('f', "Error: El directorio root no puede ser eliminado.\n");
            return false;
        }
        // Este caso es el más fácil. 
        // Ya sabemos que todo el directorio del thread tiene sentido y
        // por eso no hay que checkear nada más que el directorio
        // ingresado esté dentro del actual. Eso ya lo hace la función.
        if(strcmp(dirNames[0], ".")){
            DEBUG('f', "Error: Pasar un path o el directorio actual \".\".\n");
            return false;
        }
        name = dirNames[0];
        // Tengo que traer el directorio actual.
        // Sumarle este a su path.
        // Pasarle el path completo a AddDirFile.
        if(dirTable->CheckDirInTable(name) == -1){
            ASSERT(AddDirFile(dirNames, 0));
        }

        ASSERT(name != nullptr);
    }
    else // Acá debo checkear todo el path. 
    {
        if(!CheckPath(dirNames, subdirs))
        {
            DEBUG('f', "Error al listar el directorio. Path ingresado incorrecto.\n");
            return false;
        }

        name = dirNames[subdirs-1];
        ASSERT(name != nullptr);

        // Una vez que el path es válido, por las dudas, checkeo que todo 
        // el path esté en la dirTable.
        for (unsigned i = 0; i < subdirs; i++){
            if(dirTable->CheckDirInTable(dirNames[i]) == -1)
                ASSERT(AddDirFile(dirNames, i));
        }
    }

    dirTable->DirLock(name, ACQUIRE);
    Directory *dir = new Directory (dirTable->GetNumEntries(name));
    dir->List();
    dirTable->DirLock(name, RELEASE);
    delete dir;
    return true;
}

/// List all the files in the file system directory.
void
FileSystem::List()
{
    dirTable->DirLock("root", ACQUIRE);
    Directory *dir = new Directory(dirTable->GetNumEntries("root"));

    dir->FetchFrom(dirTable->GetDir("root"));
    dir->List();
    dirTable->DirLock("root", RELEASE);
    delete dir;
}

static bool
AddToShadowBitmap(unsigned sector, Bitmap *map)
{
    ASSERT(map != nullptr);

    if (map->Test(sector)) {
        DEBUG('f', "Sector %u was already marked.\n", sector);
        return false;
    }
    map->Mark(sector);
    DEBUG('f', "Marked sector %u.\n", sector);
    return true;
}

static bool
CheckForError(bool value, const char *message)
{
    if (!value) {
        DEBUG('f', "Error: %s\n", message);
    }
    return !value;
}

static bool
CheckSector(unsigned sector, Bitmap *shadowMap)
{
    if (CheckForError(sector < NUM_SECTORS,
                      "sector number too big.  Skipping bitmap check.")) {
        return true;
    }
    return CheckForError(AddToShadowBitmap(sector, shadowMap),
                         "sector number already used.");
}

static bool
CheckFileHeader(const RawFileHeader *rh, unsigned num, Bitmap *shadowMap)
{
    ASSERT(rh != nullptr);

    bool error = false;

    DEBUG('f', "Checking file header %u.  File size: %u bytes, number of sectors: %u.\n",
          num, rh->numBytes, rh->numSectors);
    error |= CheckForError(rh->numSectors >= DivRoundUp(rh->numBytes,
                                                        SECTOR_SIZE),
                           "sector count not compatible with file size.");
    error |= CheckForError(rh->numSectors < NUM_INDIRECT * NUM_DIRECT,
                           "too many blocks.");


    unsigned cantIndirects = DivRoundUp(rh->numSectors, NUM_DIRECT);
    unsigned located = 0;
    unsigned sectorsLeft = rh->numSectors;
    RawIndirectNode rind;
    for (unsigned i = 0; i < cantIndirects; i++) 
    {
        located = MIN(NUM_DIRECT, sectorsLeft);
        synchDisk->ReadSector(rh->dataSectors[i], (char *) &rind);
        for (unsigned j = 0; j < located; j++)
        {
            unsigned s = rind.dataSectors[j];
            error |= CheckSector(s, shadowMap);
        }
        sectorsLeft -= located;
        located = 0;
    }

    return error;
}

static bool
CheckBitmaps(const Bitmap *freeMap, const Bitmap *shadowMap)
{
    bool error = false;
    for (unsigned i = 0; i < NUM_SECTORS; i++) {
        DEBUG('f', "Checking sector %u. Original: %u, shadow: %u.\n",
              i, freeMap->Test(i), shadowMap->Test(i));
        error |= CheckForError(freeMap->Test(i) == shadowMap->Test(i),
                               "inconsistent bitmap.");
    }
    return error;
}

static bool
CheckDirectory(const RawDirectory *rd, Bitmap *shadowMap)
{
    ASSERT(rd != nullptr);
    ASSERT(shadowMap != nullptr);
    
    dirTable->DirLock("root", ACQUIRE);
    unsigned dirEntries = dirTable->GetNumEntries("root");
    bool error = false;
    unsigned nameCount = 0;
    const char *knownNames[dirEntries];

    for (unsigned i = 0; i < dirEntries; i++) {
        DEBUG('f', "Checking direntry: %u.\n", i);
        const DirectoryEntry *e = &rd->table[i];

        if (e->inUse) {
            if (strlen(e->name) > FILE_NAME_MAX_LEN) {
                DEBUG('f', "Filename too long.\n");
                error = true;
            }

            // Check for repeated filenames.
            DEBUG('f', "Checking for repeated names.  Name count: %u.\n",
                  nameCount);
            bool repeated = false;
            for (unsigned j = 0; j < nameCount; j++) {
                DEBUG('f', "Comparing \"%s\" and \"%s\".\n",
                      knownNames[j], e->name);
                if (strcmp(knownNames[j], e->name) == 0) {
                    DEBUG('f', "Repeated filename.\n");
                    repeated = true;
                    error = true;
                }
            }
            if (!repeated) {
                knownNames[nameCount] = e->name;
                DEBUG('f', "Added \"%s\" at %u.\n", e->name, nameCount);
                nameCount++;
            }

            // Check sector.
            error |= CheckSector(e->sector, shadowMap);

            // Check file header.
            FileHeader *h = new FileHeader;
            const RawFileHeader *rh = h->GetRaw();
            h->FetchFrom(e->sector);
            error |= CheckFileHeader(rh, e->sector, shadowMap);
            delete h;
        }
    }
    dirTable->DirLock("root", RELEASE);
    return error;
}

bool
FileSystem::Check()
{
    DEBUG('f', "Performing filesystem check\n");
    bool error = false;
    
    Bitmap *shadowMap = new Bitmap(NUM_SECTORS);
    
    shadowMap->Mark(FREE_MAP_SECTOR);
    shadowMap->Mark(DIRECTORY_SECTOR);

    DEBUG('f', "Checking bitmap's file header.\n");

    FileHeader *bitH = new FileHeader;
    const RawFileHeader *bitRH = bitH->GetRaw();
    bitH->FetchFrom(FREE_MAP_SECTOR);
    DEBUG('f', "  File size: %u bytes, expected %u bytes.\n"
               "  Number of sectors: %u, expected %u.\n",
          bitRH->numBytes, FREE_MAP_FILE_SIZE,
          bitRH->numSectors, FREE_MAP_FILE_SIZE / SECTOR_SIZE);
    error |= CheckForError(bitRH->numBytes == FREE_MAP_FILE_SIZE,
                           "bad bitmap header: wrong file size.");
    error |= CheckForError(bitRH->numSectors == FREE_MAP_FILE_SIZE / SECTOR_SIZE,
                           "bad bitmap header: wrong number of sectors.");
    error |= CheckFileHeader(bitRH, FREE_MAP_SECTOR, shadowMap);
    delete bitH;

    DEBUG('f', "Checking directory.\n");

    FileHeader *dirH = new FileHeader;
    const RawFileHeader *dirRH = dirH->GetRaw();
    dirH->FetchFrom(DIRECTORY_SECTOR);
    error |= CheckFileHeader(dirRH, DIRECTORY_SECTOR, shadowMap);
    delete dirH;

    Bitmap *freeMap = new Bitmap(NUM_SECTORS);
    OpenFile* freeMapFile = fileTable->GetFile("freeMap");
    freeMap->FetchFrom(freeMapFile);
    dirTable->DirLock("root", ACQUIRE);
    Directory *dir = new Directory(dirTable->GetNumEntries("root"));
    const RawDirectory *rdir = dir->GetRaw();
    dir->FetchFrom(dirTable->GetDir("root"));
    error |= CheckDirectory(rdir, shadowMap);
    delete dir;
    dirTable->DirLock("root", RELEASE);

    // The two bitmaps should match.
    DEBUG('f', "Checking bitmap consistency.\n");
    error |= CheckBitmaps(freeMap, shadowMap);
    delete shadowMap;
    delete freeMap;

    DEBUG('f', error ? "Filesystem check failed.\n"
                     : "Filesystem check succeeded.\n");

    return !error;
}

/// Print everything about the file system:
/// * the contents of the bitmap;
/// * the contents of the directory;
/// * for each file in the directory:
///   * the contents of the file header;
///   * the data in the file.
void
FileSystem::Print()
{
    FileHeader *bitH    = new FileHeader;
    FileHeader *dirH    = new FileHeader;
    
    Bitmap     *freeMap = new Bitmap(NUM_SECTORS);
    OpenFile* freeMapFile = fileTable->GetFile("freeMap");
    
    dirTable->DirLock("root", ACQUIRE);
    Directory   *dir = new Directory(dirTable->GetNumEntries("root"));
    OpenFile* directoryFile = dirTable->GetDir("root");

    printf("--------------------------------\n");
    bitH->FetchFrom(FREE_MAP_SECTOR);
    bitH->Print("Bitmap");
    
    printf("--------------------------------\n");
    dirH->FetchFrom(DIRECTORY_SECTOR);
    dirH->Print("Directory");

    printf("--------------------------------\n");
    freeMap->FetchFrom(freeMapFile);
    freeMap->Print();

    printf("--------------------------------\n");
    dir->FetchFrom(directoryFile);
    dir->Print();
    printf("--------------------------------\n");

    delete bitH;
    delete dirH;
    delete freeMap;
    delete dir;
    dirTable->DirLock("root", RELEASE);
}
