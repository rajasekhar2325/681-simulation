#include <iostream>
#include <random>
#include <fstream>
#include <string>
#include <queue>

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

enum type_of_event
{
    arrival = 1,
    quantum_done = 2,
    departure = 3,
    timeout = 4,
    contextSwitch =5
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
    thread associated_thread;
    event(type_of_event et, double est, thread t)
    {
        eventType = et;
        eventStartTime = est;
        associated_thread = t;
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
    string status;
    thread()
    {
    }
    thread(request r, int id)
    {
        req = r;
        thread_id = id;
        assigned_core_id = thread_id % no_of_cores + 1;
        status = "idle";
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

class core
{
public:
    int core_id;
    queue<thread> jobQ;
    string state;
    core(int id)
    {
        core_id = id;
    }
};

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
        int numReqCompleted = 0;
        priority_queue<event, vector<event>, compare> eventList;
        queue<request> requestQ;
        vector<core> coreList;
        for (int i = 1; i <= no_of_cores; i++)
            coreList[i] = core(i);
        threadpool tpool = threadpool();
        int no_of_req = 200;
        double simTime = 0;
        for (int i = 1; i <= no_of_req; i++)
            requestQ.push(request(i, get_random(mean_serv_time), simTime + 1, get_random(mean_timeout_time)));
        for (int i = 1; i <= no_of_req; i++)
        {
            if (tpool.poolNotEmpty())
            {
                thread t = tpool.getFreeThread(requestQ.front());
                eventList.push(event(arrival, simTime + 1, t));
                requestQ.pop();
            }
            else
                break;
        }
        while (numReqCompleted < 1000)
        {
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