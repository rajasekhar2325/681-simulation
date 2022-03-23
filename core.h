#include "event.h"

class core
{
public:
    int core_id;
    queue<thread> jobQ;
    double simTime;
    double utilization;
    int status; // 0 - idle and 1 - busy
    core(int id)
    {
        core_id = id;
        status = 0;
        simTime = 0.0;
        utilization = 0.0;
    }

    bool isEmpty()
    {
        if (jobQ.size() == 0)
            return true;
        return false;
    }
};

class Buffer
{

public:
    int sz;
    queue<event> bqu;
    Buffer()
    {
    }
    Buffer(int cap)
    {
        sz = cap;
    }

    bool isFull()
    {
        if (bqu.size() < sz)
            return true;
        return false;
    }

    bool isEmpty()
    {
        if (bqu.size() > 0)
            return false;
        return true;
    }
};