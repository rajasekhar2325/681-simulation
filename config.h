#include <iostream>
#include <string>
#include <fstream>
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
int no_of_users;
int total_requests;

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
        if (line.substr(0, line.find('=') - 1) == "users")
            no_of_users = stoi(line.substr(line.find('=') + 2));
        if (line.substr(0, line.find('=') - 1) == "simulation_end")
            total_requests = stoi(line.substr(line.find('=') + 2));
    }
    // cout << no_of_cores << endl;
    // cout << mean_serv_time << endl;
    // cout << dist_type <<  endl;
    // cout << context_switch_time << endl;
}