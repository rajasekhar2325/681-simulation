#include <iostream>
#include <random>
#include <fstream>
#include <string>
#include <queue>

#define idle 0
#define busy 1
using namespace std;

int no_of_cores;
int no_of_runs;
string dist_type;
double mean_serv_time;
double mean_arrv_time;
double mean_think_time;
double mean_timeout_time;
double time_quantum;
double context_switch_time;
int max_no_threads;
int drops;

enum type_of_event
{
    arrival = 1,
    quantum_done = 2,
    departure = 3,
    timeout = 4,
    contextSwitch = 5
};

void read_config_file()
{
    fstream config_file;
    config_file.open("config.txt", ios::in);
    string line;
    while (getline(config_file, line))
    {
        if (line.substr(0, line.find('=') - 1) == "cores")
            no_of_cores = stoi(line.substr(line.find('=') + 2));
        if (line.substr(0, line.find('=') - 1) == "runs")
            no_of_runs = stoi(line.substr(line.find('=') + 2));
        if (line.substr(0, line.find('=') - 1) == "distribution")
            dist_type = line.substr(line.find('=') + 2, line.size() - line.find('=') - 3);
        if (line.substr(0, line.find('=') - 1) == "mean_service_time")
            mean_serv_time = stoi(line.substr(line.find('=') + 2));
        if (line.substr(0, line.find('=') - 1) == "mean_arrival_time")
            mean_arrv_time = stoi(line.substr(line.find('=') + 2));
        if (line.substr(0, line.find('=') - 1) == "mean_think_time")
            mean_think_time = stoi(line.substr(line.find('=') + 2));
        if (line.substr(0, line.find('=') - 1) == "mean_timeout_time")
            mean_timeout_time = stoi(line.substr(line.find('=') + 2));
        if (line.substr(0, line.find('=') - 1) == "time_quantum")
            time_quantum = stoi(line.substr(line.find('=') + 2));
        if (line.substr(0, line.find('=') - 1) == "context_switching_time")
            context_switch_time = stoi(line.substr(line.find('=') + 2));
        if (line.substr(0, line.find('=') - 1) == "max_no_threads")
            max_no_threads = stoi(line.substr(line.find('=') + 2));
    }
    // cout << no_of_cores << endl;
    // cout << mean_serv_time << endl;
    // cout << dist_type <<  endl;
    // cout << context_switch_time << endl;
}

double get_random(double metric)
{
    default_random_engine generator;
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

class compare
{
public:
    bool operator()(event &e1, event &e2)
    {
        return e1.eventStartTime < e2.eventStartTime;
    }
};

class request
{
public:
    int req_id;
    double req_service_time;
    double req_arrival_time;
    double req_rem_serv_time;
    double req_timeout_time;
    request()
    {
    }
    request(int id, double rst, double rat, double tot)
    {
        req_id = id;
        req_service_time = rst;
        req_arrival_time = rat;
        req_timeout_time = tot;
    }
};

class thread
{
public:
    request req;
    int thread_id;
    int assigned_core_id;
    // string status;
    thread()
    {
    }
    thread(request r, int id)
    {
        req = r;
        thread_id = id;
        assigned_core_id = thread_id % no_of_cores + 1;
        // status = "busy";
    }
};

class threadpool
{
public:
    queue<int> threadQ;
    threadpool()
    {
        for (int i = 0; i < max_no_threads; i++)
            threadQ.push(i);
    }
    bool poolNotEmpty()
    {
        if (!threadQ.empty())
            return 1;
        else
            return 0;
    }
    thread getFreeThread(request req)
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

class Buffer{

public:
    int sz;
    queue<event> bqu;
    Buffer(int cap)
    {
        sz = cap;
    }

    bool isFull()
    {
        if(bqu.size() < sz)
            return true;
        return false;
    }
}

class core
{
public:
    int core_id;
    queue<thread> jobQ;
    double simTime;
    int status; // 0 - idle and 1 - busy
    core(int id)
    {
        core_id = id;
        status = 0;
        simTime = 0.0;
    }

    bool isEmpty()
    {
        if(jobQ.size() == 0)
            return true;
        return false;
    }
};

class Simulation
{
    private:
        int numReqCompleted;
        priority_queue<event, vector<event>, compare> eventList;
        Buffer buf;
        vector<core> coreList;
        threadpool tpool;
        double waitingTime;
    
    public:
        Simulation(int cores,int no_of_req)
        {
            numReqCompleted = 0;
            waitingTime = 0.0;
            coreList.reserve(cores);
            for(int i=0;i<cores;i++)
                coreList[i] = core(i);
            tpool = threadpool();
            buf = Buffer(50);

            // for (int i = 1; i <= no_of_req; i++)
            // requestQ.push(request(i, get_random(mean_serv_time), simTime + 1, get_random(mean_timeout_time)));

            for (int i = 1; i <= no_of_req; i++)
            {
                // thread t = tpool.getFreeThread(requestQ.front());
                request temp(i, get_random(mean_serv_time), simTime + 1, get_random(mean_timeout_time));
                eventList.push(event(arrival, simTime , temp));
                // requestQ.pop();
            }

        }

        void processNextEventOnCore()
        {
            event curr_event = eventList.top();
            if(curr_event.eventType == arrival)
                handleArrival();
            
            else if(curr_event.eventType == departure)
                handleDeparture();
            
            else if(curr_event.eventType == quantum_done)
                handleContextSwitch();
            else
                handleTimeout(); // need to check what logic needs to be written
        }


        void handleArrival()
        {
            event curr_event = eventList.top();
            eventList.pop();
            if(tpool.poolNotEmpty())
            {
                thread t = tpool.getFreeThread(curr_event.req);
                curr_event.thrd_id = t.id;
                int core_id = t.assigned_core_id;
                if(coreList[core_id].status == idle)
                {
                    coreList[core_id] = busy;
                    if(t.req.req_rem_serv_time <= time_quantum)
                    {
                        // schedule departure event
                        curr_event.eventType = departure;
                        curr_event.eventStartTime = coreList[core_id].simTime+curr_event.thread.req.req_rem_serv_time;
                        eventList.push(curr_event);
                        coreList[core_id].simTime += curr_event.thread.req.req_rem_serv_time;                    
                    }
                    else
                    {
                        curr_event.eventType = quantum_done;
                        curr_event.eventStartTime = coreList[core_id].simTime+quantum_done;
                        t.req.req_rem_serv_time -= time_quantum;
                        eventList.push(curr_event);
                        coreList[core_id].simTime += time_quantum;
                    }
                }
                else
                {
                    coreList[core_id].jobQ.push(t);
                }
            }
            else
            {
                if(buf.isFull())
                {
                    drops++;
                    // TODO: next request needs to be created or not?
                }
                else
                    buf.bqu.push(curr_event);
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
            int core_id = (curr_event.thread_id%4)+1;
            waitingTime += coreList[core_id].simTime  +    curr_event.thread.req.req_service_time  -   curr_event.thread.req.req_arrival_time;
            curr_event.req.req_rem_serv_time = 0;
            jobQ.pop();
            // if jobQueue is not empty schedule nextEvent
            if(coreList[core_id].isEmpty())
            {
                // TODO: what happes to simTime variable if only one request is there in each queue on next request.
                coreList[core_id].status = idle;
            }
            else
                scheduleNextEvent(core_id);
            threadpool.addToPool(curr_event.thrd_id);
            curr_event.type = arrival;
            curr_event.req.req_arrival_time = coreList[core_id].simTime+mean_think_time;
            curr_event.req.req_rem_serv_time = mean_serv_time;
            curr_event.thrd_id = -1;
            eventList.push(curr_event);
        }

        void scheduleNextEvent(int core_id)
        {
            // not able to reuse event object. so creating new event for front thread.
            thread tmp = coreList[core_id].front();
            eventType type;
            double startTime=0.0;
            if(tmp.req.req_rem_serv_time <= time_quantum)
            {
                type = departure;
                startTime = coreList[core_id].simTime+tmp.req.req_rem_serv_time;
            }
            else
            {
                type = time_quantum;
                startTime = coreList[core_id].simTime+time_quantum;
            }
            coreList[core_id] += context_switch_time;
            event curr_event(type, startTime, tmp.req);
            eventList.push(curr_event);
        }

        void handleContextSwitch()
        {
            event curr_event = eventList.top();
            eventList.pop();
            // TODO : logic for quantum done.
        }
}

int main()
{
    read_config_file();
    // cout << get_random(mean_serv_time) << endl;
    // cout << "hello world";
    // for (int i = 0; i < 5; i++)
    // {
    //     type_of_event test;
    //     test = arrival;
    //     event e1 = event(test, get_random(mean_arrv_time));
    //     // cout << get_random(mean_arrv_time);
    //     eventList.push(e1);
    // }
    // while (!eventList.empty())
    // {
    //     event test = eventList.top();
    //     cout << test.eventStartTime << endl;
    //     eventList.pop();
    // }

    while (no_of_runs--)
    {
        Simulation simobj(4,200); 
        while (numReqCompleted < 1000)
        {
            // for(int i=0;i<no_of_cores;i++)
            // {
                processNextEventOnCore();
            // }
            event curr_event = eventList.top();
            if (curr_event.eventType = arrival)
            {
                thread currThread = curr_event.associated_thread;
                coreList[currThread.assigned_core_id].jobQ.push(currThread);
            }
            // for loop of cores == 1. check core statues
            // 2. if idle == pop jobQ and assign eventType
            //3. arrival = core busy, take delta t and push to eventlist of type time qunatum; check for timeout and update badrequest.
            // 4. time quantum = check rem service time with time quantum and shedule departure event to eventlist
            // else schedule tq to eventlist
            // 5. departure = calc waiting time, free thread and add it to pool, add new arrival to eventLIst, completed++ , method to map req to thread and push to eventlist
            //
            // for(int i=0;i<no_of_cores;i++){
            //     if (coreList[i].state=="idle")
            //     {
            //         thread temp = coreList[i].jobQ.front();
            //         if (temp.req.req_rem_serv_time < time_quantum){
            //             eventList.push(event(departure,simTime,temp));
            //         }
            //     }
            // }
            if (curr_event.eventType = quantum_done)
            {
                if (curr_event.associated_thread.req.req_rem_serv_time < time_quantum)
                    eventList.push(event(departure, simTime + 1, curr_event.associated_thread));
                else
                {
                    eventList.push(event(contextSwitch, simTime + 1, curr_event.associated_thread));
                }
            }
                  if (curr_event.eventType = contextSwitch)
            {
            }
            if (curr_event.eventType = departure)
            {
            }
        }
    }
    return 0;
}