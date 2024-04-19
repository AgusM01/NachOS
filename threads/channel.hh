#ifndef NACHOS_THREADS_CHANNEL__HH
#define NACHOS_THREADS_CHANNEL__HH

#define CHANNEL_TEST

#include "thread.hh"
#include "lock.hh"
#include "condition.hh"
#include "lib/list.hh"


/// This class defines a “channel”, which has a machanism to pass 
class Channel {
public:

    Channel(const char* channelName);

    ~Channel();

    /// For debugging.
    const char *GetName() const;

    void Send(int message);
    void Receive(int *message);

private:

    /// For debugging
    const char* name;

    int* toWrite;

    Lock* mutex;
    Semaphore* readyToWrite;
    Semaphore* readyToReturn;
};


#endif
