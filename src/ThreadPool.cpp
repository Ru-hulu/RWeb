#include"ThreadPool.hpp"


static void *threadpool_thread(void *threadpool)
{
    ThreadPool* pool = reinterpret_cast<ThreadPool*>(threadpool);

    std::unique_lock<std::mutex>lg(pool->thread_pool_mutex);
    threadpoll_task_t tk;
    for(;;)
    {
        // 没有与参数列表匹配的 重载函数 "std::condition_variable::wait" 实例
        pool->thread_pool_conv.wait(lg,[pool](){return (pool->shutdown==true || pool->task_wait_count!=0);} );
        //如果通知线程关闭，或者有数据需要处理，则继续：
        if(pool->shutdown)
        {
            break;
        }
        else
        {
            tk = pool->queue[pool->head];
            pool->consume_a_task();
            lg.unlock();
        }
        (*tk.function)(tk.arg);
    }
    pool->start_thread--;
    pthread_exit(NULL);
    return(NULL);
}
ThreadPool::ThreadPool(int num_thread,int queue_size_)
{
    queue_size = queue_size_;
    queue = new threadpoll_task_t[queue_size_];
    all_thr = new pthread_t[num_thread];
    head = 0;tail = 0;
    task_wait_count = 0;
    shutdown = false;
}
void ThreadPool::consume_a_task()
{
    head ++;
    task_wait_count --;
    head = head % queue_size;
}
void ThreadPool::add_a_task()
{
    tail ++;
    task_wait_count ++;
    tail = tail % queue_size;
}
int ThreadPool::treadpoll_add_task(void (*function)(void*),void* arg)
{
    if(task_wait_count==queue_size)
    {
        std::cout<<"queue is full, we can't add a task."<<std::endl;
        return -1;
    }
        std::unique_lock<std::mutex> unl(thread_pool_mutex);
        add_a_task();
        queue[tail].function = function;
        queue[tail].arg = arg;
        unl.unlock();
        thread_pool_conv.notify_one();
    return 1;
}
bool ThreadPool::InitialPool()
{
    //if(pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void*)pool) != 0)     
    for(int i=0;i<sizeof(all_thr)/sizeof(pthread_t);i++)
    {
        if(pthread_create(&all_thr[i],NULL,threadpool_thread,this)!=0)
        {
            shutdown = true;
            return false;
        }
        start_thread++;        
    }
    return true;
}

ThreadPool::~ThreadPool()
{
    for(int i=0;i<start_thread;i++)
    {
        thread_pool_conv.notify_one();//逐个通知每个线程，释放线程资源
        pthread_join(all_thr[i],NULL);
        //先释放所有的线程
    }
    delete[] queue;
    delete[] all_thr;
}