#include<iostream>
#include<mutex>
#include<thread>
#include<pthread.h>
#include<condition_variable>

#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP
struct threadpoll_task_t
{
    void (*function)(void*);
    void* arg;
};
class ThreadPool
{
    public:
        ThreadPool(int num_thread,int que_size);
        ~ThreadPool();
        bool InitialPool();
        void consume_a_task();
        void add_a_task();
        int treadpoll_add_task(void (*function)(void*),void* arg);
        std::condition_variable thread_pool_conv;
        std::mutex thread_pool_mutex;
        int start_thread;
        threadpoll_task_t* queue;//所有需要处理的任务都在里面
        pthread_t* all_thr;//所有的线程都在这里
        bool shutdown;
        int head; //queue循环数组头部
        int tail; //queue的尾部
        int task_wait_count; //有多少个任务等待处理，后续应该被分配到每个线程中   
        int queue_size; // 任务队列的长度是固定的。因为一个线程需要处理很多个连接的任务，所以有任务队列

};
#endif