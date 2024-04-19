#include "channel.hh"
#include "condition.hh"
#include "semaphore.hh"
#include <type_traits>

Channel::Channel(const char* channelName) {
    name = channelName;
    toWrite = NULL;
    mutex = new Lock(0);
    readyToWrite = new Semaphore(0, 0);
    readyToReturn = new Semaphore(0, 0);

}

Channel::~Channel() {
    delete mutex;
    delete readyToWrite;
    delete readyToReturn;
}

void
Channel::Send(int message) {
    //Esperamos a Receive que nos habilite toWrite
    readyToWrite->P();

    //Buffer listo, escribimos
    *toWrite = message;

    //Escritura hecha, retornamos en conjunto
    readyToReturn->V();

}

void
Channel::Receive(int* message) {
    //Esperamos a que no haya otro Receive
    mutex->Acquire();

    //Disponemos en buffer en Write
    toWrite = message;

    //Send ya nos puede mandar el mensaje
    readyToWrite->V();

    //Esperamos a que Send nos mande para retornar.
    readyToReturn->P();

    //Ya tenemos el dato en *message, habilitamos nuevos Receive
    mutex->Release();
}