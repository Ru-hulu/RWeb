#include<iostream>
#include<mutex>
#include<thread>
#include<pthread.h>
#include<condition_variable>
#include<vector>
#include<stdexcept>

#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

const int THREADPOOL_INVALID = -1;
const int THREADPOOL_LOCK_FAILURE = -2;
const int THREADPOOL_QUEUE_FULL = -3;
const int THREADPOOL_SHUTDOWN = -4;
const int THREADPOOL_THREAD_FAILURE = -5;
const int THREADPOOL_GRACEFUL = 1;
const int THREADPOOL_CAPACITY = 4;
class Task_Function_Arg
{
    public:
        Task_Function_Arg(){};
        void (*function)(void*);
        void* arg;
};
class Thread_Pack
{
    public:
        Thread_Pack(int thread_id_);//这里面需要构造thread_c
        ~Thread_Pack();//需要释放thread_c, join也在这里面
        int thread_id = 0;
        pthread_t* thread_c;
};
class ThreadPool
{
    public:
        ssize_t now_all_con = 0;
        ThreadPool(int num_thread,int que_size);
        ~ThreadPool();
        bool InitialPool();
        void consume_a_task_update_idex();
        void add_a_task_update_idex();
        int treadpoll_add_task(void (*function)(void*),void* arg);
        int thread_id_counter = 0;
        std::condition_variable thread_pool_conv;//分发任务的时候用的条件变量
        std::mutex thread_pool_mutex;//处理任务队列的时候使用的锁
        int start_thread;
        std::vector<Task_Function_Arg*> task_queue;
        //所有需要处理的任务都在里面
        std::vector<Thread_Pack*> all_thr;
        //所有的线程都在这里
        bool shutdown;//线程池是否已经完成删除
        int head; //task_queue循环数组头部
        int tail; //task_queue的尾部
        int task_wait_count; //有多少个任务等待处理，后续应该被分配到每个线程中   
        int queue_size; // 任务队列的长度是固定的。因为一个线程需要处理很多个连接的任务，所以有任务队列
};
#endif