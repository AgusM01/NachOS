/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_FILESYS_DIRECTORYENTRY__HH
#define NACHOS_FILESYS_DIRECTORYENTRY__HH


/// For simplicity, we assume file names are <= 9 characters long.
const unsigned FILE_NAME_MAX_LEN = 9;
const unsigned DIR_NAME_MAX_LEN = FILE_NAME_MAX_LEN;

/// The following class defines a "directory entry", representing a file in
/// the directory.  Each entry gives the name of the file, and where the
/// file's header is to be found on disk.
///
/// Internal data structures kept public so that Directory operations can
/// access them directly.
class DirectoryEntry {
public:
    /// Is this directory entry in use?
    bool inUse;
    /// Location on disk to find the `FileHeader` for this file. -> 'FileHeader' es i-nodo.
    /// Es la ubicación en el disco para encontrar el i-nodo ya que en NachOS es 1:1, no tenemos tabla de i-nodos.
    /// Cada i-nodo entra en exactamente 1 sector. Tienen el mismo tamaño.
    /// El sector 0 podría contener el i-nodo 0, el sector 1 podría contener el i-nodo 1,
    /// el sector N podría contener el i-nodo N. Podría también contener datos.
    ///
    /// El sector 0, contiene el i-nodo del bit-map. La metadata del bit-map.
    /// El sector 1, contiene el i-nodo del directorio raíz. La metadata del directorio raíz.
    ///
    /// Acceder al directorio raíz y al bit-map es acceder a archivos comunes.
    unsigned sector;
    /// Text name for file, with +1 for the trailing `'\0'`.
    char name[FILE_NAME_MAX_LEN + 1];
};


#endif
