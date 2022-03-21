#include "thread.h"

enum type_of_event
{
    arrival = 1,
    quantum_done = 2,
    departure = 3,
    timeout = 4
};


class event
{
public:
    type_of_event eventType;
    double eventStartTime;
    request req;
    int thrd_id;
    event(type_of_event et, double est, request t)
    {
        eventType = et;
        eventStartTime = est;
        req = t;
    }
};
