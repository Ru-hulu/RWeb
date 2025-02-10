#include<iostream>
#include<string> 
#include<string.h> 
#include"AsyncLogging.hpp"
#include<fcntl.h>
#include<stdio.h>
#include <unistd.h>
#include<time.h>
#include<assert.h>
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
AsyncLogging::AsyncLogging()
{
    rollFd();
    currbuff.reset(new Buff());
    nextbuff.reset(new Buff());
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
    +"-"+std::to_string(hour)+"-"+std::to_string(min)+"-"+std::to_string(sec);
    log_fd = open(file_name.c_str(),O_RDWR|O_CREAT);
    if(log_fd<0)
    {
        std::cout<<"roll_fd failed"<<std::endl;
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
    return true;
}
void AsyncLogging::ThreadWriteLog()
{
    BuffPtr Buff1(new Buff());
    BuffPtr Buff2(new Buff());
    std::vector<BuffPtr> Buff2Write;
    ssize_t logfile_write = 0;
    for(;;)
    {
        {
            std::unique_lock<std::mutex> lk(wb_mutex);
            update_log_file.wait_for(lk,std::chrono::seconds(wait_time),
            [this](){return update_check();});
            buffs_.push_back(std::move(currbuff));
            Buff2Write.swap(buffs_);
            currbuff = std::move(Buff1);
            if(nextbuff==nullptr)
            {
                nextbuff = std::move(Buff2);
            }
            set_empty();
            lk.unlock();
        }
        if(Buff2Write.size()>25)
        {
            char too_much_d[40];
            sprintf(too_much_d,"To much log data!\n\0");
            write(log_fd,too_much_d,strlen(too_much_d));
            Buff2Write.erase(Buff2Write.begin()+2,Buff2Write.end());
        }
        for(auto &b:Buff2Write)
        {
            ssize_t nw_byte= 1024-b->avail();
            logfile_write += nw_byte;
            write(log_fd,b->get_buff_(),nw_byte);
        }
        if(fsync(log_fd)==-1)
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
        if(logw_upbound<=logfile_write||LogExpired())
        {
            rollFd();
        }
        //对文件描述符检查并且更新
    }
}