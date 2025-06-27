//// Métodos para la estructura indirect-node.
////
//// Un nodo de indirección del Filesystem es un nodo el 
//// cuál únicamente guarda sectores de un archivo.
////
//// El i-node real es el file_header el cual representa un archivo
//// y tiene su metadata.
//// El mismo contiene los sectores del archivo mediante doble indirección.
////
//// El indirect-node contiene un array de la cantidad de punteros directos
//// a sectores (NUM_DIRECT).
////
//// Cada puntero del i-nodo apunta a un indirect-node el cual contiene
//// NUM_DIRECT sectores.
////
//// Un indirect-node cabe enteramente en un sector.
//
//#include "indirect_node.hh"
//#include "lib/utility.hh"
//#include "threads/system.hh"
//
//#include <ctype.h>
//#include <stdio.h>
//
//// Inicializa un indirect-node.
//// Es llamado desde el Allocate del file_header.
//// Reserva en el bitmap la cantidad de sectores necesarios
//// pedido por el file_header.
//// 
//// * 'freeMap' es el bit map de sectores libres.
//// * 'sectors' es la cantidad de sectores a ser reservados.
//bool
//IndirectNode::Allocate(Bitmap* freeMap, unsigned nSectors, unsigned thisSector)
//{
//    ASSERT(freeMap != nullptr);
//
//    // Tamaño de la porción de archivo que contiene.
//    raw.numBytes = nSectors * SECTOR_SIZE;
//
//    // Cantidad de sectores que contiene.
//    raw.numSectors = nSectors;
//
//    // Sector donde está alojado.
//    raw.rawSector = thisSector;
//
//    if (freeMap->CountClear() < raw.numSectors)
//        return false; // No hay espacio para los sectores.
//
//    for (unsigned i = 0; i < raw.numSectors; i++) {
//        raw.dataSectors[i] = freeMap->Find();
//    } 
//    return true;
//}
//
//// Elimina todo el espacio ocupado por los sectores que contiene 
//// este indirect-node.
//// 
//// * 'freeMap' es el bit map de los sectores libres.
//void 
//IndirectNode::Deallocate(Bitmap* freeMap)
//{
//    ASSERT(freeMap != nullptr);
//
//    for (unsigned i = 0; i < raw.numSectors; i++){
//        ASSERT(freeMap->Test(raw.dataSectors[i]));
//        freeMap->Clear(raw.dataSectors[i]);
//    }
//}
//
//// Trae los contenidos del indirect-node desde el disco.
//void
//IndirectNode::FetchFrom()
//{
//    synchDisk->ReadSector(raw.rawSector, (char *) &raw);
//}
//
//// Escribe los contenidos modificados de este indirect-node de nuevo al disco.
//void
//IndirectNode::WriteBack()
//{
//    synchDisk->WriteSector(raw.rawSector, (char *) &raw);
//}
//
//unsigned
//IndirectNode::ByteToSector(unsigned ByteSector)
//{
//    return raw.dataSectors[ByteSector];
//}
//
//unsigned
//IndirectNode::FileLength() const
//{
//    return raw.numBytes;
//}
//
//void
//IndirectNode::Print()
//{
//   for (unsigned i = 0; i < raw.numSectors; i++)
//       printf("%u ", raw.dataSectors[i]);
//   return;
//}
//
//void
//IndirectNode::PrintContent()
//{
//
//    char *data = new char [raw.numBytes];
//    for (unsigned i = 0; i < raw.numSectors; i++)
//    {
//        printf("    contents of block %u:\n", raw.dataSectors[i]);
//        synchDisk->ReadSector(raw.dataSectors[i], data);
//        for (unsigned j = 0; j < raw.numBytes; j++)
//        {
//            if (isprint(data[j])) {
//                printf("%c", data[j]);
//            } else {
//                printf("\\%X", (unsigned char) data[j]);
//            }
//        }
//        printf("\n");
//    }
//    
//    delete [] data;
//    return;
//}
//
////void
////IndirectNode::SetIndirecSector(unsigned IndirecSector)
////{
////    ASSERT(IndirecSector <= NUM_SECTORS);
////    sector = IndirecSector;
////    return;
////}
////
////unsigned
////IndirectNode::GetIndirecSector()
////{
////    return sector;
////}
//
//unsigned
//IndirectNode::GetNumberSectorsStored()
//{
//    return raw.numSectors;
//}
//
//const RawIndirectNode *
//IndirectNode::GetRaw() const
//{
//    return &raw;
//}
//
