#include "request.h"
#include "config.h"
#include <queue>

class thread
{
public:
    request *req;
    int thread_id;
    int assigned_core_id;
    // string status;
    thread() {}
    thread(request *r, int id)
    {
        this->req = r;
        thread_id = id;
        assigned_core_id = (thread_id % no_of_cores);
        // status = "busy";
    }
};


class threadpool
{
public:
    queue<int> threadQ;
    threadpool()
    {
        for (int i = 1; i <= max_no_threads; i++)
            threadQ.push(i);
    }
    bool poolNotEmpty()
    {
        if (!threadQ.empty())
            return 1;
        else
            return 0;
    }
    thread getFreeThread(request *req)
    {
        thread freeThread = thread(req, threadQ.front());
        threadQ.pop();
        return freeThread;
    }
    void addToPool(int id)
    {
        threadQ.push(id);
    }
};
