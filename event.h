#include "thread.h"

enum type_of_event
{
    Arrival = 1,
    Quantum_done = 2,
    Departure = 3
};

string eventName[]={"Arrival","Quantum_done","Departure","Timeout"};

class event
{
public:
    type_of_event eventType;
    double eventStartTime;
    request *req;
    int thrd_id;
    event(type_of_event et, double est, request *t)
    {
        eventType = et;
        eventStartTime = est;
        this->req = t;
    }
};
