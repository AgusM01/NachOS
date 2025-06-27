//// Estructura de datos que representa un nodo de indirección
//// del Filesystem.
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
//#ifndef NACHOS_FILESYS_INDIRECTNODE_HH
//#define NACHOS_FILESYS_INDIRECTNODE_HH
//
//#include "raw_indirect_node.hh"
//#include "lib/bitmap.hh"
//
//class IndirectNode {
//public:
//    
//    // Inicializa un IndirectNode el cual va a guardar
//    // size sectores.
//    bool Allocate(Bitmap *bitMap, unsigned nSectors, unsigned thisSector);
//
//    // Elimina los sectores de datos que contiene del archivo.
//    // Esta función se llama externamente del Deallocate del 
//    // FileHeader.
//    void Deallocate(Bitmap *bitMap);
//   
//    // Inicializa el indirect-node desde el Disco.
//    void FetchFrom();
//
//    // Escribe las modificaciones al disco.
//    void WriteBack();
//
//    // Convierte un byte de offset dentro del archivo al
//    // sector del disco conteniendo el byte.
//    unsigned ByteToSector(unsigned ByteSector);
//
//    // Retorna el tamaño del pedazo de archivo que contiene
//    // en bytes.
//    unsigned FileLength() const;
//    
//    /// Imprime qué sectores que contiene.
//    void Print();
//
//    // Imprime el contenido de todo lo que contiene.
//    void PrintContent();
//    
//   // // Setea el sector donde está guardado el indirect-node
//   // void SetIndirecSector(unsigned IndirecSector);
//
//   // // Devuelve el sector donde está guardado el indirect-node
//   // unsigned GetIndirecSector();
//    
//    // Devuelve la cantidad de sectores que contiene.
//    unsigned GetNumberSectorsStored();
//
//    // Trae el raw indirect-node.
//    const RawIndirectNode *GetRaw() const;
//private:
//    RawIndirectNode raw;
//};
//#endif /* NACHOS_FILESYS_INDIRECTNODE_HH */
