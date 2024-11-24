#include"requestData.hpp"
#include<unistd.h>
myTimer::myTimer(requestData* rqtp_,size_t timeoutms):rqtp(rqtp_)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    expired_time = tv.tv_sec*1000 + tv.tv_usec/1000 + timeoutms;
    return;
}
myTimer::~myTimer()
{
    //析沟函数中需要释放requestData;
    if(rqtp!=nullptr)
    {
        delete rqtp;
    }    
    return;
}
size_t myTimer::get_expire_time()
{
    return expired_time;
}
void myTimer::clearRqt()
{
    rqtp = nullptr;
}

bool myTimer::isValid()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    size_t nowt = tv.tv_sec*1000 + tv.tv_usec/1000;
    if(nowt>expired_time || rqtp==nullptr)
    {
        return false;
    }
    return true;
}
requestData::requestData(int fd_,int epoll_fd_)
{
    fd = fd_;
    epoll_fd = epoll_fd_;
}
requestData::~requestData()
{
    int ev_id = EPOLLIN|EPOLLET|EPOLLONESHOT;
    epoll_del(epoll_fd,fd,ev_id);//删除epoll_fd 与 fd的关联
    close(fd);//关闭文件描述符
    if(mtimer!=nullptr)
    {
        seperateTimer();
    }
}
void requestData::setTimer(myTimer* tm_)
{
    mtimer = tm_;
}
void requestData::seperateTimer()
{
    mtimer->clearRqt();
    mtimer = nullptr;
}
void requestData::setFd(int fd_)
{
    fd = fd_;
}

int requestData::getFd()
{
    return fd;
}

void myHandlefunc(void* arg)
{

}