#include<iostream>
#include<string> 
#include<mutex>
#include<vector>
#include<memory>
#include<condition_variable>
#ifndef ASYNCLOGGING
#define ASYNCLOGGING
class Buff
{
    public:
        Buff():cur_(buff_){};
        int avail();//
        void reset_cur();
        void buff_write(char*,int);
        const char * get_buff_(){return buff_;};
    private:
        char buff_[1024];
        char* cur_;
};
class AsyncLogging
{
    public:
        typedef std::unique_ptr<Buff> BuffPtr;
        AsyncLogging();//log时间初始化,current初始化
        static void* ThreadWriteLog(void* arg_);//真正将数据写入到log中的线程函数
        void BuffWriteFunc(char*,int);//将缓冲区写入的函数
        void rollFd();//如果日志文件太大或者时间很长，需要更新日志文件。
        bool update_check();
        void set_empty();
        void set_full();
        bool LogExpired();//看当前Log的是否已经存在太长时间， 如果时间太长就创建新的log.
    private:
        pthread_t* write_thread;
        BuffPtr currbuff;
        BuffPtr nextbuff;
        time_t log_init_t;
        ssize_t logw_upbound = 1000000;
        int wait_time = 3;
        std::vector<BuffPtr> buffs_;
        int log_fd=-1;
        bool full_flag = false;
        std::mutex wb_mutex;
        std::condition_variable update_log_file;
};
#endif