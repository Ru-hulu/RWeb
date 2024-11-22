#include"epoll.hpp"
int epoll_init()
{
    int epoll_Fd = epoll_create(100);
    if(epoll_Fd<0)return -1;
    events_get = new epoll_event[MAXEVENT];
    if(events_get==nullptr)
        return -1;        
    return epoll_Fd;
}

int epoll_add(int epoll_fd_,void* request_data,int fd_,uint32_t events_id)
{
    struct epoll_event ev;
    ev.data.ptr = request_data;
    ev.events = events_id;
    int rqt = epoll_ctl(epoll_fd_,EPOLL_CTL_ADD,fd_,&ev);
    if(rqt<0)
    {
        perror("epoll_add error");
        return -1;        
    }
    return 0;
}

int epoll_mod(int epoll_fd_,void* request_data,int fd_,uint32_t events_id)
{
    struct epoll_event ev;
    ev.events = events_id;
    ev.data.ptr = request_data;
    int rqt = epoll_ctl(epoll_fd_,EPOLL_CTL_MOD,fd_,&ev);
    if(rqt<0)
    {
        perror("epoll_mod error");
        return -1;        
    }
    return 0;
}

int epoll_del(int epoll_fd_,void* request_data,int fd_,uint32_t events_id)
{
    struct epoll_event ev;
    ev.data.ptr = request_data;
    ev.events = events_id;
    if(epoll_ctl(epoll_fd_,EPOLL_CTL_DEL,fd_,&ev)<0)
    {
        perror("epoll_del error");
        return -1;        
    }
    return 0;
}

int my_epoll_wait(int epoll_fd_,struct epoll_event* events, uint32_t time_out,uint32_t maxevents)
{
    int rt = epoll_wait(epoll_fd_,events,maxevents,time_out);
    if(rt<0)
    {
        perror("epoll_wait error");
    }
    return rt;
    //如果正常
}