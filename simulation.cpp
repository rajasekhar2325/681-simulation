#include <random>
#include "core.h"
#include <iostream>
#include <iomanip>

using namespace std;
#define idle  0
#define busy  1

ofstream trace;
ofstream result;

// function which returns random values for input parameters with particular distribution function 
double get_random(double metric, int flag)
{
    if (!flag)
        return metric;
    random_device rd;
    default_random_engine generator(rd());
    if (dist_type == "uniform")
    {
        uniform_real_distribution<double> distribution(metric, 1);
        return distribution(generator);
    }
    if (dist_type == "normal")
    {
        normal_distribution<double> distribution(metric, 20);
        return distribution(generator);
    }
    if (dist_type == "exponential")
    {
        exponential_distribution<double> distribution(1/metric);
        return distribution(generator);
    }
    return 0;
}

// Custom comparator for heap
class compare
{
public:
    bool operator()(event &e1, event &e2)
    {
        if (e1.eventStartTime == e2.eventStartTime)
            return e1.req->req_id > e2.req->req_id;
        return e1.eventStartTime > e2.eventStartTime;
    }
};

//logging function
void printlog(double simTime, type_of_event et, int reqID, int coreID, double servTime, double remTime)
{
    if (print_log)
    trace << left << setw(15) << simTime << setw(15) << eventName[et - 1] << setw(15) << reqID << setw(10) << coreID << setw(15) << servTime << setw(15) << remTime << endl;
}

class Simulation
{
public:
    int numReqCompleted;
    int goodRequests;
    int drops;
    priority_queue<event, vector<event>, compare> eventList;
    Buffer buf;
    vector<core> coreList;
    threadpool tpool;
    double waitingTime;
    double responseTime;
    Simulation(int cores, int no_of_req)
    {
        numReqCompleted = 0;
        goodRequests = 0;
        drops = 0;
        waitingTime = 0.0;
        responseTime = 0.0;
        coreList.reserve(cores);
        for (int i = 0; i < cores; i++)
            coreList.push_back(core(i));
        tpool = threadpool();
        buf = Buffer(bsz);

        for (int i = 1; i <= no_of_req; i++)
        {
            request *temp = new request(i, get_random(mean_serv_time, 1), 0, get_random(mean_timeout_time, 1));
            eventList.push(event(Arrival, 0, temp));
        }
    }

    // Based on the Event, this function calls specific function associated with it 
    void processNextEventOnCore()
    {
        event curr_event = eventList.top();
        switch (curr_event.eventType)
        {
        case Arrival:
            handleArrival();
            break;

        case Departure:
            handleDeparture();
            break;

        case Quantum_done:
            handleContextSwitch();
            break;
        }
    }

// Handles arrival event of request(user)
    void handleArrival()
    {

        // get the first event in eventList
        event curr_event = eventList.top();
        eventList.pop();

        // assign a thread to it
        if (tpool.poolNotEmpty())
        {
            thread tmp = tpool.getFreeThread(curr_event.req);
            curr_event.thrd_id = tmp.thread_id;
            int core_id = (tmp.thread_id) % no_of_cores;
            coreList[core_id].simTime = curr_event.eventStartTime;

            printlog(coreList[core_id].simTime, curr_event.eventType, curr_event.req->req_id, core_id, curr_event.req->req_service_time, curr_event.req->req_rem_serv_time);

            if (coreList[core_id].status == idle)
            {
                coreList[core_id].status = busy;
                type_of_event type;
                double eventTime = 0.0;

                // schedule departure event
                if (tmp.req->req_rem_serv_time <= time_quantum)
                {
                    type = Departure;
                    eventTime = coreList[core_id].simTime + curr_event.req->req_rem_serv_time;
                    curr_event.req->req_rem_serv_time = 0;
                }
                // schedule quantum_done event
                else
                {
                    type = Quantum_done;
                    eventTime = coreList[core_id].simTime + time_quantum;
                    curr_event.req->req_rem_serv_time -= time_quantum;
                }

                curr_event.eventType = type;
                curr_event.eventStartTime = eventTime;
                eventList.push(curr_event);
            }

            coreList[core_id].jobQ.push(tmp);
        }
        // push to buffer as there are no available threads
        else
        {
            // update drops is buffer is full and schedule a new request
            if (buf.isFull())
            {
                drops++;
                curr_event.req->req_arrival_time = curr_event.eventStartTime + get_random(mean_think_time, 1);
                curr_event.eventStartTime = curr_event.req->req_arrival_time;
                curr_event.req->req_service_time = curr_event.req->req_rem_serv_time = get_random(mean_serv_time, 1);
                curr_event.req->req_timeout_time = get_random(mean_timeout_time, 1);
                eventList.push(curr_event);
            }
            // push to buffer is buffer has space
            else
            {
                buf.bqu.push(curr_event);
            }
        }
    }

    // Handles departure event for the request
    void handleDeparture()
    {
        /* 1. increment request complete count by 1
        2. update waiting time
        3. schedule nextArrival of request
        4. free thread and assign the free thread to next request in request queue*/

        // increment completed requests by 1
        event curr_event = eventList.top();
        eventList.pop();
        numReqCompleted++;

        // update goodput
        if(curr_event.eventStartTime-curr_event.req->req_arrival_time <= curr_event.req->req_timeout_time)
            goodRequests++;

        // update waiting time, response time
        int core_id = (curr_event.thrd_id % no_of_cores);
        coreList[core_id].simTime =  curr_event.eventStartTime;
        waitingTime += curr_event.eventStartTime - curr_event.req->req_service_time - curr_event.req->req_arrival_time;
        responseTime += coreList[core_id].simTime - curr_event.req->req_arrival_time;
        coreList[core_id].utilization += curr_event.req->req_service_time;

        printlog(coreList[core_id].simTime, curr_event.eventType, curr_event.req->req_id, core_id, curr_event.req->req_service_time, curr_event.req->req_rem_serv_time);

        coreList[core_id].jobQ.pop();

        // if jobQueue is not empty schedule nextEvent
        if (coreList[core_id].isEmpty())
        {
            coreList[core_id].status = idle;
        }
        else
            scheduleNextEvent(core_id);

        // add the departured event thread id to the free pool
        tpool.addToPool(curr_event.thrd_id);

        // schedule a new request with some think time
        curr_event.eventType = Arrival;
        curr_event.req->req_arrival_time = curr_event.eventStartTime + get_random(mean_think_time, 1);
        curr_event.eventStartTime = curr_event.req->req_arrival_time;
        curr_event.req->req_service_time = curr_event.req->req_rem_serv_time = get_random(mean_serv_time, 1);
        curr_event.req->req_timeout_time = get_random(mean_timeout_time, 1);
        eventList.push(curr_event);

        // process buf, add buf top to thread and push to core queue
        if (not buf.isEmpty())
        {
            event e1 = buf.bqu.front();
            buf.bqu.pop();
            thread tmp = tpool.getFreeThread(e1.req);
            e1.thrd_id = tmp.thread_id;
            core_id = e1.thrd_id%no_of_cores;
            if(coreList[core_id].status == idle)
            {
                coreList[core_id].status = busy;
                type_of_event type;
                double eventTime = 0.0;
                if (tmp.req->req_rem_serv_time <= time_quantum)
                {
                    type = Departure;
                    eventTime = coreList[core_id].simTime + e1.req->req_rem_serv_time + context_switch_time;
                    e1.req->req_rem_serv_time = 0;
                }
                else
                {
                    type = Quantum_done;
                    eventTime = coreList[core_id].simTime + time_quantum + context_switch_time;
                    e1.req->req_rem_serv_time -= time_quantum;
                }

                e1.eventType = type;
                e1.eventStartTime = eventTime;
                eventList.push(e1);
                printlog(coreList[core_id].simTime, e1.eventType, e1.req->req_id, core_id, e1.req->req_service_time, e1.req->req_rem_serv_time);
            }

            coreList[tmp.thread_id%no_of_cores].jobQ.push(tmp);
        }
    }


    // schedules next job in core queue which is next to currently departing job

    void scheduleNextEvent(int core_id)
    {
        thread tmp = coreList[core_id].jobQ.front();
        type_of_event type;
        double eventTime = 0.0;
        if (tmp.req->req_rem_serv_time <= time_quantum)
        {
            type = Departure;
            eventTime = coreList[core_id].simTime + tmp.req->req_rem_serv_time + context_switch_time;
            tmp.req->req_rem_serv_time = 0;
        }
        else
        {
            type = Quantum_done;
            eventTime = coreList[core_id].simTime + time_quantum + context_switch_time;
            tmp.req->req_rem_serv_time -= time_quantum;
        }

        event next_event(type, eventTime, tmp.req);
        next_event.thrd_id = tmp.thread_id;
        eventList.push(next_event);
    }

    // Handles context switching event(quantum done event) for the request
    void handleContextSwitch()
    {
        event curr_event = eventList.top();
        eventList.pop();
        int core_id = (curr_event.thrd_id % no_of_cores);
        coreList[core_id].simTime = curr_event.eventStartTime;

        printlog(coreList[core_id].simTime, curr_event.eventType, curr_event.req->req_id, core_id, curr_event.req->req_service_time, curr_event.req->req_rem_serv_time);

        // Moving front job in jobQ to back
        thread tmp = coreList[core_id].jobQ.front();
        coreList[core_id].jobQ.pop();
        coreList[core_id].jobQ.push(tmp);

        // Processing the next job in jobQ
        tmp = coreList[core_id].jobQ.front();

        curr_event.req = tmp.req;
        curr_event.thrd_id = tmp.thread_id;

        if (tmp.req->req_rem_serv_time <= time_quantum)
        {
            curr_event.eventType = Departure;
            curr_event.eventStartTime = coreList[core_id].simTime + curr_event.req->req_rem_serv_time + context_switch_time;
            eventList.push(curr_event);
            curr_event.req->req_rem_serv_time = 0;
        }
        else
        {
            curr_event.eventType = Quantum_done;
            curr_event.eventStartTime = coreList[core_id].simTime + time_quantum + context_switch_time;
            curr_event.req->req_rem_serv_time -= time_quantum;
            eventList.push(curr_event);
        }
    }
};



int main()
{
    result.open("result.txt");
    trace.open("log.txt");
    read_config_file();

    trace << left << setw(15) << "Time" << setw(15) << "Event Type" << setw(15) << "Request ID" << setw(10) << "Core ID" << setw(15) << "Service Time" << setw(15) << "Remaining Time" << endl;
    
    while (no_of_runs--)
    {
        Simulation simobj(no_of_cores, no_of_users);

        // simluation loop
        while (simobj.numReqCompleted < total_requests)
        {
            simobj.processNextEventOnCore();
        }

        // calculating metrics
        double total_utilization = 0.0;
        double total_simulation_time = 0.0;
        double avg_utilization = 0.0;
        double avg_simulation_time = 0.0;
        for(int i=0;i<no_of_cores;i++)
        {
            total_utilization += simobj.coreList[i].utilization;
            total_simulation_time += simobj.coreList[i].simTime;
        }
        avg_simulation_time = total_simulation_time/(double)no_of_cores;
        avg_utilization = total_utilization/(double)no_of_cores;
        result << "Avg_server_utilization: " << avg_utilization/avg_simulation_time << endl;
        result << "Total_request_drops: " << simobj.drops << endl;
        result << "Good_throughput: " << ((double)simobj.goodRequests/avg_simulation_time)*1000 << endl;
        result << "Bad_throughput: " << ((double)(simobj.numReqCompleted - simobj.goodRequests)/avg_simulation_time)*1000 << endl;
        result << "Total_throughput: " << ((double)(simobj.numReqCompleted)/avg_simulation_time)*1000 << endl;
        result << "Avg_response_time: " << (simobj.responseTime / total_requests)/(double)1000 << endl;
        result << "Avg_waiting_time: " << (simobj.waitingTime/total_requests)/(double)1000 << endl;
    }
    trace.close();
    result.close();
    return 0;
}
