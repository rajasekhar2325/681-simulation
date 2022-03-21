#include <random>
#include "core.h"
#include <iostream>

using namespace std;
#define idle 0
#define busy 1
int drops;

// enum type_of_event
// {
//     arrival = 1,
//     quantum_done = 2,
//     departure = 3,
//     timeout = 4
// };

double get_random(double metric)
{
    random_device rd;
    default_random_engine generator(rd());
    // cout << dist_type << " " << dist_type.size() << endl;
    if (dist_type == "constant")
    {
        normal_distribution<double> distribution(metric, 1);
        return distribution(generator);
    }
    if (dist_type == "normal")
    {
        normal_distribution<double> distribution(metric, 1);
        return distribution(generator);
    }
    if (dist_type == "exponential")
    {
        exponential_distribution<double> distribution(metric);
        return distribution(generator);
    }
    return 0;
}

class compare
{
public:
    bool operator()(event &e1, event &e2)
    {
        return e1.eventStartTime > e2.eventStartTime;
    }
};

class Simulation
{
public:
    int numReqCompleted;
    priority_queue<event, vector<event>, compare> eventList;
    Buffer buf;
    vector<core> coreList;
    threadpool tpool;
    double waitingTime;
    double simTime = 0;
    Simulation(int cores, int no_of_req)
    {
        numReqCompleted = 0;
        waitingTime = 0.0;
        coreList.reserve(cores);
        for (int i = 0; i < cores; i++)
            coreList.push_back(core(i));
        tpool = threadpool();
        buf = Buffer(50);

        // for (int i = 1; i <= no_of_req; i++)
        // requestQ.push(request(i, get_random(mean_serv_time), simTime + 1, get_random(mean_timeout_time)));

        for (int i = 1; i <= no_of_req; i++)
        {
            // thread t = tpool.getFreeThread(requestQ.front());
            request temp(i, get_random(mean_serv_time), i - 1, get_random(mean_timeout_time));
            eventList.push(event(arrival, i - 1, temp));
            // requestQ.pop();
        }
        printeventList();
    }

    void processNextEventOnCore()
    {
        event curr_event = eventList.top();
        if (curr_event.eventType == arrival)
            handleArrival();

        else if (curr_event.eventType == departure)
            handleDeparture();

        else if (curr_event.eventType == quantum_done)
            handleContextSwitch();

        else if (curr_event.eventType == timeout)
            handleTimeout(); // need to check what logic needs to be written
    }

    void handleArrival()
    {
        event curr_event = eventList.top();
        eventList.pop();
        cout << "handlearrival: currevent: " << curr_event.req.req_id << " eventStarttime: " << curr_event.eventStartTime << endl;
        if (tpool.poolNotEmpty())
        {
            thread t = tpool.getFreeThread(curr_event.req);
            curr_event.thrd_id = t.thread_id;
            cout << "assigned thread to event.  ThreadID " << curr_event.thrd_id << endl;
            int core_id = t.assigned_core_id;
            // if (coreList[core_id].status == idle)
            // {
                coreList[core_id].status = busy;
                t.req.req_rem_serv_time -= time_quantum;
                tpool.activeThreads[t.thread_id]=t;
                if (t.req.req_rem_serv_time <= time_quantum)
                {
                    // schedule departure event
                    cout << "adding departure event" << endl;
                    curr_event.eventType = departure;
                    curr_event.eventStartTime = simTime + t.req.req_rem_serv_time;
                    curr_event.req.req_rem_serv_time = 0;
                    eventList.push(curr_event);
                    simTime += t.req.req_rem_serv_time;
                }
                else
                {
                    cout << "adding quantum-done event" << endl;
                    curr_event.eventType = quantum_done;
                    curr_event.eventStartTime = simTime + time_quantum;
                    curr_event.req.req_rem_serv_time -= time_quantum;
                    eventList.push(curr_event);
                    simTime += time_quantum;
                }
           
    
                coreList[core_id].jobQ.push(t);
            
        }
        else
        {
            if (buf.isFull())
            {
                drops++;
                // TODO: next request needs to be created or not?
            }
            else
                buf.bqu.push(curr_event); //raj; event or req // when it will be processed?
        }
        printeventList();
    }

    void printeventList()
    {
        priority_queue<event, vector<event>, compare> temp = eventList;
        while (!temp.empty())
        {
            cout << temp.top().req.req_id << " " << temp.top().eventType << " " << temp.top().eventStartTime << " " << temp.top().req.req_rem_serv_time << "||";
            temp.pop();
        }
    }

    void handleDeparture()
    {
        // increement request complete count by 1
        // update waiting time
        // schedule next arrival of request
        // free thread and assign the free thread to next request in request queue

        event curr_event = eventList.top();
        eventList.pop();
        numReqCompleted++;
        int core_id = (curr_event.thrd_id % no_of_cores);
        cout << "handledeparture: currevent: " << curr_event.req.req_id << " " << simTime << endl;
        waitingTime += simTime - curr_event.req.req_service_time - curr_event.req.req_arrival_time; //raj + to -
        cout << "waiting time: " << waitingTime << endl;
        curr_event.req.req_rem_serv_time = 0;
        coreList[core_id].jobQ.pop(); // raj: req?
        // if jobQueue is not empty schedule nextEvent
        if (coreList[core_id].isEmpty())
        {
            // TODO: what happes to simTime variable if only one request is there in each queue on next request.
            coreList[core_id].status = idle;
        }
        else
            scheduleNextEvent(core_id);

        tpool.addToPool(curr_event.thrd_id);
        curr_event.eventType = arrival;
        curr_event.req.req_arrival_time = simTime + get_random(mean_think_time);
        curr_event.req.req_service_time = curr_event.req.req_rem_serv_time = get_random(mean_serv_time);
        curr_event.req.req_timeout_time = get_random(mean_timeout_time);
        curr_event.thrd_id = -1;
        eventList.push(curr_event);
        printeventList();

        // process buf, add buf top to thread and push to
        if (!buf.isEmpty())
        {
            event e1 = buf.bqu.front();
            if (tpool.poolNotEmpty())
            {
                thread t1 = tpool.getFreeThread(e1.req);
                coreList[t1.assigned_core_id].jobQ.push(t1);
            }
        }
    }

    void handleTimeout()
    {
    }

    void scheduleNextEvent(int core_id)
    {
        // not able to reuse event object. so creating new event for front thread.
        thread tmp = coreList[core_id].jobQ.front();
        type_of_event type;
        double startTime = 0.0;
        cout << "schedulenextevent: " << tmp.req.req_id << endl;
        if (tmp.req.req_rem_serv_time <= time_quantum)
        {
            type = departure;
            startTime = simTime + tmp.req.req_rem_serv_time;
        }
        else
        {
            type = quantum_done;
            startTime = simTime + time_quantum;
        }
        simTime += context_switch_time;
        event next_event = event(type, startTime, tmp.req);
        cout << "pushing event: " << type << endl;
        eventList.push(next_event);
        //raj: should we pop thread in jobq?
    }

    void handleContextSwitch()
    {
        event curr_event = eventList.top();
        eventList.pop();
        // TODO : logic for quantum done.

        cout << "handleContextswitch: currevent: " << curr_event.req.req_id << " " << endl;
        int core_id = (curr_event.thrd_id % no_of_cores);
        // cout << tpool.threadFromId(curr_event.thrd_id).req.req_rem_serv_time << endl;
        coreList[core_id].jobQ.push(tpool.threadFromId(curr_event.thrd_id));

        thread t1 = coreList[core_id].jobQ.front();
        cout << "t1 rem serv time: " << t1.req.req_rem_serv_time << endl;
        if (t1.req.req_rem_serv_time <= time_quantum)
        {
            // schedule departure event
            cout << "adding departure event " << simTime << endl;
            curr_event.eventType = departure;
            curr_event.eventStartTime = simTime + curr_event.req.req_rem_serv_time;
            curr_event.req.req_rem_serv_time = 0;
            coreList[core_id].jobQ.front().req.req_rem_serv_time = 0;
            eventList.push(curr_event);
            simTime += curr_event.req.req_rem_serv_time;
        }
        else
        {
            //
            cout << "adding quantum-done event" << simTime << endl;
            curr_event.eventType = quantum_done;
            curr_event.eventStartTime = simTime + quantum_done;
            curr_event.req.req_rem_serv_time -= time_quantum;
            coreList[core_id].jobQ.front().req.req_rem_serv_time -= time_quantum;
            eventList.push(curr_event);
            simTime += time_quantum;
        }
        simTime += context_switch_time;
        printeventList();
    }
};

int main()
{
    freopen("log.txt", "w", stdout);
    read_config_file();

    while (no_of_runs--)
    {
        Simulation simobj(no_of_cores, 2);
        while (simobj.numReqCompleted < 4)
        {
            cout << "simtime: " << simobj.simTime << endl;
            simobj.processNextEventOnCore();
        }
    }
    return 0;
}