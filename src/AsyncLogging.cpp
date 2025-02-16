#include<iostream>
#include<string> 
#include<string.h> 
#include"AsyncLogging.hpp"
#include<fcntl.h>
#include<stdio.h>
#include <unistd.h>
#include<time.h>
#include<assert.h>
#include<memory>
inline int Buff::avail()
{
    return (1024-(cur_-buff_));
}
//写之前保证空间足够,文件指针会移动
void Buff::buff_write(char* s,int l)
{
    memcpy(cur_,s,l);
    cur_ += l;
}
void Buff::reset_cur()
{
    cur_ = buff_;
}
AsyncLogging::~AsyncLogging()
{
    alive = false;
    pthread_join(*write_thread,NULL);
    delete write_thread;
}   
AsyncLogging& AsyncLogging::getitem()
{
    static AsyncLogging async;
    return async;
}
AsyncLogging::AsyncLogging()
{
    rollFd();
    currbuff.reset(new Buff());
    nextbuff.reset(new Buff());
    write_thread  = new pthread_t;
    alive = true;
    pthread_create(write_thread,NULL,ThreadWriteLog,this);
}
//更新日志文件以及时间。
void AsyncLogging::rollFd()
{
    if(log_fd!=-1)
    {
        close(log_fd);
    }
    time_t tt;
    time(&tt);
    log_init_t = tt;
    struct tm* st = localtime(&tt);
    int year = st->tm_year+1900;
    int moth = st->tm_mon+1;
    int day = st->tm_mday;
    int hour = st->tm_hour;
    int min = st->tm_min;
    int sec = st->tm_sec;
    std::string file_name = "/home/r/Mysoftware/RWeb/log/"
        +std::to_string(year)+"-"+std::to_string(moth)+"-"+std::to_string(day)
    +"-"+std::to_string(hour)+"-"+std::to_string(min)+"-"+std::to_string(sec)
    +".txt";
    log_fd = open(file_name.c_str(),O_RDWR|O_CREAT,0666);
    if(log_fd<0)
    {
        std::cout<<file_name<<std::endl;
        std::cout<<errno<<std::endl;
        abort();
    }
}
void AsyncLogging::BuffWriteFunc(char* d,int len)
{
    assert(len<=1024);
    std::unique_lock<std::mutex>lk(wb_mutex);
    if(currbuff->avail()>=len)
    {
        currbuff->buff_write(d,len);
    }
    else//currbuff空间不够放下log
    {
        int avail_size = currbuff->avail();
        currbuff->buff_write(d,avail_size);//先写一部分
        d += avail_size;//偏移指针
        len -= avail_size;
        //现在currbuff一定满了
        buffs_.push_back(std::move(currbuff));
        if(nextbuff==nullptr)
        {
            currbuff.reset(new Buff());
            currbuff->buff_write(d,len);
        }
        else if(nextbuff!=nullptr)
        {
            currbuff = std::move(nextbuff);
            currbuff->buff_write(d,len);
        }
        set_full();
        lk.unlock();
        update_log_file.notify_one();//触发更新
    }
    //写到CurrentBlock里面
}
inline void AsyncLogging::set_full()
{
    full_flag=true;
}
inline void AsyncLogging::set_empty()
{
    full_flag=false;
}
inline bool AsyncLogging::update_check()
{
    return full_flag;
}
bool AsyncLogging::LogExpired()
{
    time_t tt;
    time(&tt);
    struct tm* now_t= localtime(&tt);
    struct tm* ini_t= localtime(&log_init_t);
    if(now_t->tm_wday==ini_t->tm_wday)return false;
    else if(now_t->tm_hour+24-ini_t->tm_hour>=24)
    {
        std::cout<<"time expired "<<std::endl;
        return true;
    }
}
void* AsyncLogging::ThreadWriteLog(void* arg_)
{
    BuffPtr Buff1(new Buff());
    BuffPtr Buff2(new Buff());
    AsyncLogging* arg  = reinterpret_cast<AsyncLogging*>(arg_);
    std::vector<BuffPtr> Buff2Write;
    ssize_t logfile_write = 0;
    while(arg->alive)
    {
        {
            std::unique_lock<std::mutex> lk(arg->wb_mutex);
            arg->update_log_file.wait_for(lk,std::chrono::seconds(arg->wait_time),
            [arg](){return arg->update_check();});
            arg->buffs_.push_back(std::move(arg->currbuff));
            Buff2Write.swap(arg->buffs_);
            arg->currbuff = std::move(Buff1);
            if(arg->nextbuff==nullptr)
            {
                arg->nextbuff = std::move(Buff2);
            }
            arg->set_empty();
            lk.unlock();
        }
        if(Buff2Write.size()>25)
        {
            char too_much_d[40];
            sprintf(too_much_d,"To much log data!\n\0");
            write(arg->log_fd,too_much_d,strlen(too_much_d));
            for(int cctb=1;cctb<Buff2Write.size();cctb++)
            {
                if(Buff2Write[cctb]->isempty())continue;
                if(Buff2Write[cctb]->get_end()=='\n')
                {
                    Buff2Write.erase(Buff2Write.begin()+cctb+1,Buff2Write.end());
                    break;
                }
            }
        }
        for(auto &b:Buff2Write)
        {
            ssize_t nw_byte= 1024-b->avail();
            logfile_write += nw_byte;
            write(arg->log_fd,b->get_buff_(),nw_byte);
        }
        if(fsync(arg->log_fd)==-1)
        {
            std::cout<<"fsync(log_fd) error !"<<std::endl;
            abort();
        }
        if(Buff1==nullptr)
        {
            Buff1 = std::move(Buff2Write.back());
            Buff2Write.pop_back();
            Buff1->reset_cur();
        }
        if(Buff2==nullptr)
        {
            Buff2 = std::move(Buff2Write.back());
            Buff2Write.pop_back();
            Buff2->reset_cur();
        }
        if(arg->logw_upbound<=logfile_write||arg->LogExpired())
        {
            arg->rollFd();
            logfile_write = 0;
        }
        Buff2Write.clear();
        //对文件描述符检查并且更新
    }
    //全部是智能指针，不需要释放
    pthread_exit(NULL);
    return NULL;
}