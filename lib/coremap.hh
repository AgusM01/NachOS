/// Estructura de datos que define el coremap. 
/// Una especie de tabla de paginación invertida. Con él no sólo se podrá saber qué marcos físicos están libres, sino también a qué espacio de direcciones corresponde un marco ocupado y cuáles de sus páginas virtuales aloja.

#ifndef NACHOS_LIB_COREMAP__HH
#define NACHOS_LIB_COREMAP__HH

#include "utility.hh"
#include "filesys/open_file.hh"

#include "lib/list.hh"


struct corestruct {
    unsigned pid; // Espacio de direcciones.
    unsigned vpn; // Pagina alojada.
    bool used;    // Usado o no
};

typedef struct corestruct CoreStruct;

#ifdef PRPOLICY_CLOCK

#endif
class CoreMap{

public:

    /// Inicializar el CoreMap con 'nitems'todos inicializados en 0.
    CoreMap(unsigned nitems);
    
    /// Destructor del CoreMap.
    ~CoreMap();
    
/// Set the “nth” bit.
    void Mark(unsigned which, unsigned vpn, int proc_id);

    /// Clear the “nth” bit.
    void Clear(unsigned which);

    /// Is the “nth” bit set?
    bool Test(unsigned which) const;
    
    
    /// Return the index of a clear bit, and as a side effect, set the bit.
    ///
    /// If no bits are clear, return -1.
    int Find(unsigned vpn, int proc_id);

    /// Return the number of clear bits.
    unsigned CountClear() const;

    /// Print contents of bitmap.
    void Print() const;

    /// Fetch contents from disk.
    ///
    /// Note: this is not needed until the *FILESYS* assignment, when we will
    /// need to read and write the bitmap to a file.
    void FetchFrom(OpenFile *file);

    /// Write contents to disk.
    ///
    /// Note: this is not needed until the *FILESYS* assignment, when we will
    /// need to read and write the bitmap to a file.
    void WriteBack(OpenFile *file) const;
   
    /// Devuelve un numero de marco que será la víctima a reemplazo.
    int PickVictim();
    
    unsigned GetPid(unsigned which);

    unsigned GetVpn(unsigned which);

private:

    /// Number of bits in the bitmap.
    unsigned numBits;

    /// Number of words of bitmap storage (rounded up if `numBits` is not a
    /// multiple of the number of bits in a word).
    unsigned numWords;

    /// Bit storage.
    CoreStruct* map;
    
    #ifdef PRPOLICY_FIFO
        List<unsigned> *fifo_list;    
    #endif

};



#endif
