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

enum type_of_event
{
    arrival = 1,
    quantum_done = 2,
    departure = 3,
    timeout = 4
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

    event(type_of_event et, double est)
    {
        eventType = et;
        eventStartTime = est;
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

priority_queue<event, vector<event>, compare> eventList;

class request
{
public:
    int req_id;
    double req_arrival_time;
    double req_service_time;
    double req_rem_serv_time;
    double req_timeout_time;
};

class thread
{
public:
    request req;
    int thread_id;
    int assigned_core_id;
    string status;

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
    int max_no_threads = 100;
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
};

int main()
{
    queue<request> requestQueue;
    read_config_file();
    cout << get_random(mean_serv_time) << endl;
    cout << "hello world";
    for (int i = 0; i < 5; i++)
    {
        type_of_event test;
        test = arrival;
        event e1 = event(test, get_random(mean_arrv_time));
        // cout << get_random(mean_arrv_time);
        eventList.push(e1);
    }
    while (!eventList.empty())
    {
        event test = eventList.top();
        cout << test.eventStartTime << endl;
        eventList.pop();
    }
    return 0;
}