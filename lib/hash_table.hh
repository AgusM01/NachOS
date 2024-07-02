#ifndef __HASH_TABLE__HH
#define __HASH_TABLE__HH

typedef struct controlNode ControlNode;

typedef struct node {
    node* next;  
    void* data;
    char* name;
} Node;

typedef    void(*FuncionDestructora)(void *dato);
typedef    int(*FuncionComparadora)(const char* name1, const char* name2);
typedef    unsigned (*FuncionHash)(const char* dato);
typedef    void(*FuncionVisitante)(void *dato);
//typedef    void*(*FuncionCopiadora)(void *dato);

template <class H>
class HashTable{
    
public:

    HashTable(FuncionDestructora destroyFunc, FuncionComparadora compFunc, FuncionHash hashFunc, FuncionVisitante visitFunc);
 
    
    ~HashTable();

    void H_Add(H item, const char* name);

    H H_Get(const char* name);

    bool H_Delete(const char* name);

private:

    Node** tabla;

    int Tcount;

    int Tcapacity;
     
    FuncionDestructora destroyFunc;
    
    FuncionComparadora compFunc;

    FuncionHash hashFunc; 

    FuncionVisitante visitFunc;

    //FuncionCopiadora copyFunc;
    //void ReHash(); <- Ejercicio para el lector.
};

#endif
