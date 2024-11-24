#include<iostream>
#include <sys/time.h>
#include <mutex>
#include<vector>
#include"epoll.hpp"
#ifndef REQUESTDATA_HPP
#define REQUESTDATA_HPP

extern std::mutex mutex_timer_q;
class requestData;
class myTimer;
void myHandlefunc(void* arg);
class requestData
{
    public:
        requestData(int fd_,int epoll_fd_);
        ~requestData();
        void seperateTimer();
        myTimer* mtimer;
        void setFd(int fd_);
        int getFd();
        void setTimer(myTimer* tm_);
    private:
        int fd;
        int epoll_fd;
};

class myTimer
{
    /* data */
    public:
        myTimer(requestData* rqtp_,size_t timeoutms);
        ~myTimer();
        size_t get_expire_time();
        bool isValid();
        void clearRqt();
        size_t expired_time;
    private:
        requestData* rqtp;
        bool deleted;//是否允许删除，就是定时器已经和requetsdata无关联，可以判定为失效        
};
struct timecmp
{
   bool operator()(const myTimer* a,const myTimer* b)const;
};
#endif