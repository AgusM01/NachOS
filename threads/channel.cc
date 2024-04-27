#include "channel.hh"
#include "system.hh"
#include <stdlib.h>

Channel::Channel(const char* channelName) {
    name = channelName;
    lockName = channelName ? concat("Lock ", channelName) : nullptr;
    semWName = channelName ? concat("SemW ", channelName) : nullptr;
    semRName = channelName ? concat("SemR ", channelName) : nullptr;
    toWrite = nullptr;
    mutex = new Lock(lockName);
    readyToWrite = new Semaphore(semWName, 0);
    readyToReturn = new Semaphore(semRName, 0);

}

Channel::~Channel() {
    delete mutex;
    delete readyToWrite;
    delete readyToReturn;
    free(lockName);
    free(semRName);
    free(semWName);
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

    //Esperamos a Receive que nos habilite toWrite
    readyToWrite->P();

    ASSERT(toWrite != nullptr);
    //Buffer listo, escribimos
    *toWrite = message;

    //Restablecemos toWrite a nullptr para checkeos;
    toWrite = nullptr;

    //Escritura hecha, retornamos en conjunto
    readyToReturn->V();

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
    ASSERT(toWrite == nullptr);

    //Disponemos el buffer en Write
    toWrite = message;

    //Send ya nos puede mandar el mensaje
    readyToWrite->V();

    //Esperamos a que Send nos mande para retornar.
    readyToReturn->P();

    //Checkeamos que toWrite esté vacío
    ASSERT(toWrite == nullptr);

    //Ya tenemos el dato en *message, habilitamos nuevos Receive
    mutex->Release();
}