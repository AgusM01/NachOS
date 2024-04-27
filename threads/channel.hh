#ifndef NACHOS_THREADS_CHANNEL__HH
#define NACHOS_THREADS_CHANNEL__HH

#define CHANNEL_TEST

#include "lock.hh"
#include "condition.hh"

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
    char* lockName;
    char* semWName;
    char* semRName;

    int* toWrite;

    Lock* mutex;
    Semaphore* readyToWrite;
    Semaphore* readyToReturn;
};


#endif
