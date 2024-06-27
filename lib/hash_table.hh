#ifndef __HASH_TABLE__HH
#define __HASH_TABLE__HH

typedef struct controlNode ControlNode;

typedef struct node {
    node* next;  
    ControlNode* data;
} Node;


template <class H>
class HashTable{
    
public:

    HashTable();
    
    ~HashTable();

    void H_Add(H item);

    H H_Get(char* name);

    bool H_Delete(char* name);

private:
    Node** tabla;

    unsigned HashFunc(char* name);

    int Tcount;

    int Tcapacity;

    //void ReHash(); <- Ejercicio para el lector.
};

#endif
