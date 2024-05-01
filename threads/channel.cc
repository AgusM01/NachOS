#include "channel.hh"
#include "system.hh"
#include <stdlib.h>

Channel::Channel(const char* channelName) {
    name = channelName;
    lockName = channelName ? concat("Lock ", channelName) : nullptr;
    condSendName = channelName ? concat("varSend ", channelName) : nullptr;
    condRecvName = channelName ? concat("varRecv ", channelName) : nullptr;
    semName = channelName ? concat("SemfinishWrt ", channelName) : nullptr;

    toWrite = nullptr;
    mutex = new Lock(lockName);
    condSend = new Condition(condSendName, mutex);
    condRecv = new Condition(condRecvName, mutex);
    finishWrt = new Semaphore(semName, 0);
    
}

Channel::~Channel() {
    delete mutex;
    delete condSend;
    delete condRecv;
    delete finishWrt;
    free(lockName);
    free(semName);
    free(condSendName);
    free(condRecvName);
    
}

const char *
Channel::GetName() const
{
    return name;
}

void
Channel::Send(int message) {
    DEBUG('c', "Hago Send en %s, soy %s\n",
        this->GetName(),
        currentThread->GetName()
    );

    mutex->Acquire();
    
    //Esperamos a Receive que nos habilite toWrite
    while (toWrite == nullptr){
        condSend->Wait();
    }
    
    ASSERT(toWrite != nullptr);
    
    //Buffer listo, escribimos
    *toWrite = message;

    //Restablecemos toWrite a nullptr para checkeos;
    toWrite = nullptr;
    
    // Si hay un recv esperando a que escriba le aviso.
    condRecv->Signal();

    mutex->Release();

    //Escritura hecha, retornamos en conjunto
    finishWrt->V();
}

void
Channel::Receive(int* message) {

    DEBUG('c', "Hago Receive en %s, soy %s\n",
        this->GetName(),
        currentThread->GetName()
    );

    //Tiene que haber un buffer disponible
    ASSERT(message != nullptr);

    //Esperamos a que no haya otro Receive
    mutex->Acquire();

    //Checkeamos que toWrite esté vacío
    while (toWrite != nullptr){
        condRecv->Wait();
    }

    //Disponemos el buffer en Write
    toWrite = message;

    //Send ya nos puede mandar el mensaje
    condSend->Signal();
    
    mutex->Release();

    //Esperamos a que Send nos mande para retornar.
    finishWrt->P();
}
