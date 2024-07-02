/// Funciones correspondientes a la tabla hash creada.

#include "hash_table.hh"
#include "filesys/file_system.hh"
#include <cstring>

#define SIZE 10;

template <class H>
HashTable<H>::HashTable(FuncionDestructora destroyFunc, FuncionComparadora compFunc, FuncionHash hashFunc, FuncionVisitante visitFunc){
   
    Tcapacity = SIZE;
    tabla = new Node*[Tcapacity]; 
    Tcount = 0; 
    for (int i = 0; i < Tcapacity ; i++){
        tabla[i] = new Node;
        tabla[i]->next = nullptr;
        tabla[i]->data = nullptr;
    }

    this->destroyFunc = destroyFunc;
    this->compFunc = compFunc;
    this->hashFunc = hashFunc;
    this->visitFunc = visitFunc;

    return;
}

template <class H>
HashTable<H>::~HashTable(){

    for (int i = 0; i < Tcapacity; i++)
        for(Node* temp = tabla[i]; temp != nullptr; temp = temp->next){
            // No borrar el nombre del nodo ya que es el mismo puntero que esta en el data
            destroyFunc(temp->data);
            delete temp;

        }

    delete tabla;
    return;
}

template <class H>
void 
HashTable<H>::H_Add(H item, const char* name){
    
    Node* new_node = new Node;
    unsigned pos = HashFunc(item) % Tcapacity;
    int len_name = strlen(name);
    new_node->name = new char[len_name + 1];
    memcpy(new_node->name, name, len_name + 1);
    new_node->data = item;
    new_node->next = tabla[pos];
    tabla[pos] = new_node;
    Tcount++;
    Tcapacity++;
    return;
}

template <class H>
H 
HashTable<H>::H_Get(const char* name){
    unsigned pos = hashFunc(name) % Tcapacity;
    for(Node* temp = tabla[pos]; temp != nullptr; temp = temp->next){
        if(compFunc(temp->name, name))
            return temp->data;
    }
    return nullptr;
}

template <class H>
bool
HashTable<H>::H_Delete(const char* name){
    
    unsigned pos = hashFunc(name) % Tcapacity;
    
    if(tabla[pos] == nullptr)
        return false;

    if(compFunc(tabla[pos]->name, name)){
        Node* to_delete = tabla[pos];
        tabla[pos] = to_delete->next;
        destroyFunc(to_delete->data);
        delete to_delete->name;
        delete to_delete;
        return true;
    }
    else{
        for(Node* temp = tabla[pos]; temp != nullptr; temp = temp->next){
            
            if(temp->next == nullptr)
                return false;

            if(compFunc(temp->next->name, name)){
                Node* to_delete = temp->next;
                temp->next = to_delete->next;
                destroyFunc(to_delete->data);
                delete to_delete->name;
                delete to_delete;
                return true;
            }
        }
    }

    return false;
}

/*template <class H>
unsigned
HashTable<H>::HashFunc(char* Iname){

    unsigned hash = 5381;
    
    int c;

    char* str = Iname;
    
    while((c = *str++))
        hash = ((hash << 5) + hash) +c;

    return hash;
}*/
