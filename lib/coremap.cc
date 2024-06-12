/// Routines to manage a bitmap -- an array of bits each of which can be
/// either on or off.  Represented as an array of integers.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "coremap.hh"
#include "system_dep.hh"

#include <cstdint>
#include <stdio.h>

/// Initialize a bitmap with `nitems` bits, so that every bit is clear.  It
/// can be added somewhere on a list.
///
/// * `nitems` is the number of bits in the bitmap.
CoreMap::CoreMap(unsigned nitems)
{
    ASSERT(nitems > 0);

    numBits  = nitems; /// Cantidad de bits 
    numWords = DivRoundUp(numBits, BITS_IN_WORD); /// Calcula cuantos enteros son (32 bits -> 4 bytes)
    map      = new CoreStruct [numWords]; /// Crea un array con la cantidad de enteros que le dió 
    for (unsigned i = 0; i < numBits; i++) {
        Clear(i);
    }
}

/// De-allocate a CoreMap.
CoreMap::~CoreMap()
{
    delete [] map;
}

/// Set the “nth” bit in a coremap.
///
/// * `which` is the number of the bit to be set.
void
CoreMap::Mark(unsigned which)
{
    ASSERT(which < numBits);
    map[which].used = true;
}

/// Clear the “nth” bit in a coremap.
///
/// * `which` is the number of the bit to be cleared.
void
CoreMap::Clear(unsigned which)
{
    ASSERT(which < numBits);
    map[which].used = false;
}

/// Return true if the “nth” bit is set.
///
/// * `which` is the number of the bit to be tested.
bool
CoreMap::Test(unsigned which) const
{
    ASSERT(which < numBits);
    return map[which].used;
}

/// Return the number of the first bit which is clear.  As a side effect, set
/// the bit (mark it as in use).  (In other words, find and allocate a bit.)
///
/// If no bits are clear, return -1.
int
CoreMap::Find(unsigned vpn, int proc_id)
{
    for (unsigned i = 0; i < numBits; i++) {
        if (!Test(i)) {
            map[i].used = true;
            map[i].pid = proc_id;
            map[i].vpn = vpn;
            return i;
        }
    }
    return -1;
}

/// Return the number of clear bits in the coremap.  (In other words, how many
/// bits are unallocated?)
unsigned
CoreMap::CountClear() const
{
    unsigned count = 0;

    for (unsigned i = 0; i < numBits; i++) {
        if (!Test(i)) {
            count++;
        }
    }
    return count;
}

/// Print the contents of the coremap, for debugging.
///
/// Could be done in a number of ways, but we just print the indexes of all
/// the bits that are set in the bitmap.
void
CoreMap::Print() const
{
    printf("Bitmap bits set:\n");
    for (unsigned i = 0; i < numBits; i++) {
        if (Test(i)) {
            printf("%u ", i);
        }
    }
    printf("\n");
}

int
CoreMap::PickVictim()
{
   return SystemDep::Random() % numBits; 
}



/// Initialize the contents of a coremap from a Nachos file.
///
/// Note: this is not needed until the *FILESYS* assignment.
///
/// * `file` is the place to read the bitmap from.
void
CoreMap::FetchFrom(OpenFile *file)
{
    ASSERT(file != nullptr);
    file->ReadAt((char *) map, numWords * sizeof (CoreStruct), 0);
}

/// Store the contents of a CoreMap to a Nachos file.
///
/// Note: this is not needed until the *FILESYS* assignment.
///
/// * `file` is the place to write the bitmap to.
void
CoreMap::WriteBack(OpenFile *file) const
{
    ASSERT(file != nullptr);
    file->WriteAt((char *) map, numWords * sizeof (CoreStruct), 0);
}
