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
    Simulation(int cores, int no_of_req)
    {
        numReqCompleted = 0;
        waitingTime = 0.0;
        coreList.reserve(cores);
        for (int i = 0; i < cores; i++)
            coreList.push_back(core(i));
        tpool = threadpool();
        buf = Buffer(50);

        for (int i = 1; i <= no_of_req; i++)
        {
            request *temp = new request(i, get_random(mean_serv_time), i - 1, get_random(mean_timeout_time));
            cout << temp << "\n";
            eventList.push(event(arrival, i - 1, temp));
        }
        printeventList();
    }

    void processNextEventOnCore()
    {
        event curr_event = eventList.top();
        switch(curr_event.eventType)
        {
        case arrival:
            handleArrival();
            break;

        case departure:
            handleDeparture();
            break;

        case quantum_done:
            handleContextSwitch();
            break;

        // else if (curr_event.eventType == timeout)
        default:
            handleTimeout(); // need to check what logic needs to be written
            break;
        }
    }

    void handleArrival()
    {
        event curr_event = eventList.top();
        eventList.pop();
        cout << "handlearrival: currevent: " << curr_event.req->req_id << " eventStarttime: " << curr_event.eventStartTime << endl;
        if (tpool.poolNotEmpty())
        {
            thread tmp = tpool.getFreeThread(curr_event.req);
            curr_event.thrd_id = tmp.thread_id;
            cout << "assigned thread to event.  ThreadID " << curr_event.thrd_id << endl;
            int core_id = tmp.assigned_core_id;
            if (coreList[core_id].status == idle)
            {
                coreList[core_id].status = busy;
                // tmp.req->req_rem_serv_time -= time_quantum;
                type_of_event type;
                double eventTime = 0.0;
                cout << "schedulenextevent: " << tmp.req->req_id << endl;
                coreList[core_id].simTime = max(coreList[core_id].simTime, curr_event.eventStartTime);
                if (tmp.req->req_rem_serv_time <= time_quantum)
                {
                    cout << "adding departure event" << endl;
                    type = departure;
                    eventTime = coreList[core_id].simTime + curr_event.req->req_rem_serv_time;
                    coreList[core_id].simTime += curr_event.req->req_rem_serv_time;
                    curr_event.req->req_rem_serv_time = 0;
                }
                else
                {
                    cout << "adding quantum-done event" << endl;
                    type = quantum_done;
                    eventTime = coreList[core_id].simTime + time_quantum;
                    curr_event.req->req_rem_serv_time -= time_quantum;
                    coreList[core_id].simTime += time_quantum;
                }
                curr_event.eventType = type;
                curr_event.eventStartTime = eventTime;
                eventList.push(curr_event);
            }
            coreList[core_id].jobQ.push(tmp);
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
            cout << temp.top().req->req_id << " " << temp.top().eventType << " " << temp.top().eventStartTime << " " << temp.top().req->req_rem_serv_time << "||";
            temp.pop();
        }
        cout << "\n";
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
        cout << "handledeparture: currevent: " << curr_event.req->req_id << " " << coreList[core_id].simTime << endl;
        waitingTime += coreList[core_id].simTime - curr_event.req->req_service_time - curr_event.req->req_arrival_time; //raj + to -
        cout << coreList[core_id].simTime  << " " << curr_event.req->req_service_time << " " <<  curr_event.req->req_arrival_time << "\n";
        cout << "waiting time: " << waitingTime << endl;
        curr_event.req->req_rem_serv_time = 0;
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
        curr_event.req->req_arrival_time = curr_event.eventStartTime + get_random(mean_think_time);
        curr_event.eventStartTime  = curr_event.req->req_arrival_time;
        curr_event.req->req_service_time = curr_event.req->req_rem_serv_time = get_random(mean_serv_time);
        curr_event.req->req_timeout_time = get_random(mean_timeout_time);
        curr_event.thrd_id = -1;
        eventList.push(curr_event);
        printeventList();

        // process buf, add buf top to thread and push to
        if (!buf.isEmpty())
        {
            event e1 = buf.bqu.front();
            buf.bqu.pop();
            if (tpool.poolNotEmpty())
            {
                thread t1 = tpool.getFreeThread(e1.req);
                coreList[t1.assigned_core_id].jobQ.push(t1);
                // check cpu core status and process.EG: logic is similar to arrival request and thread is available from pool
            }
        }
    }

    void handleTimeout()
    {
    }

    void scheduleNextEvent(int core_id)
    {
        // not able to reuse event object. so creating new event for front thread.
        cout << core_id << " " << "Hello\n";
        thread tmp = coreList[core_id].jobQ.front();
        type_of_event type;
        double eventTime = 0.0;
        cout << "schedulenextevent: " << tmp.req->req_id << endl;
        if (tmp.req->req_rem_serv_time <= time_quantum)
        {
            type = departure;
            eventTime = coreList[core_id].simTime + tmp.req->req_rem_serv_time;
            coreList[core_id].simTime += tmp.req->req_rem_serv_time;
        }
        else
        {
            type = quantum_done;
            eventTime = coreList[core_id].simTime + time_quantum;
            coreList[core_id].simTime += time_quantum;
        }
        coreList[core_id].simTime += context_switch_time;
        event next_event = event(type, eventTime, tmp.req);
        cout << "pushing event: " << type << endl;
        eventList.push(next_event);
        //raj: should we pop thread in jobq?
    }

    void handleContextSwitch()
    {
        // logic for quantum done.
        event curr_event = eventList.top();
        eventList.pop();
        cout << "handleContextswitch: currevent: " << curr_event.req->req_id << " " << endl;
        int core_id = (curr_event.thrd_id % no_of_cores);
        // Moving front job in jobQ to back
        thread tmp = coreList[core_id].jobQ.front();
        coreList[core_id].jobQ.pop();
        coreList[core_id].jobQ.push(tmp);

        // Processing the next job in jobQ
        tmp = coreList[core_id].jobQ.front();
        coreList[core_id].simTime = max(coreList[core_id].simTime, tmp.req->req_arrival_time);
        cout << "tmp rem serv time: " << tmp.req->req_rem_serv_time << endl;
        curr_event.req = tmp.req;
        if (tmp.req->req_rem_serv_time <= time_quantum)
        {
            // schedule departure event
            cout << "adding departure event " << coreList[core_id].simTime << endl;
            curr_event.eventType = departure;
            curr_event.eventStartTime = coreList[core_id].simTime + curr_event.req->req_rem_serv_time;
            // coreList[core_id].jobQ.front().req->req_rem_serv_time = 0; //Check: same request object in event and thread right?
            eventList.push(curr_event);
            coreList[core_id].simTime += curr_event.req->req_rem_serv_time;
            curr_event.req->req_rem_serv_time = 0;
        }
        else
        {
            cout << "adding quantum-done event" << coreList[core_id].simTime << endl;
            curr_event.eventType = quantum_done;
            curr_event.eventStartTime = coreList[core_id].simTime + quantum_done;
            curr_event.req->req_rem_serv_time -= time_quantum;
            // coreList[core_id].jobQ.front().req->req_rem_serv_time -= time_quantum;
            eventList.push(curr_event);
            coreList[core_id].simTime += time_quantum;
        }
        coreList[core_id].simTime += context_switch_time;
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
            for(int i=1;i<=no_of_cores;i++)
                cout << "simtime on core " << i << " : " << simobj.coreList[i].simTime << endl;
            simobj.processNextEventOnCore();
        }
    }
    return 0;
}