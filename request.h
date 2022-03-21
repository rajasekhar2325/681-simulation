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
        req_rem_serv_time = rst;
    }
};