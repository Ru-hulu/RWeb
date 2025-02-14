#include"ThreadPool.hpp"
#include"MemoryPool.hpp"
#include"requestData.hpp"
#include"Logger.hpp"
Thread_Pack::Thread_Pack(int thread_id_)
{
    thread_id = thread_id_;
    thread_c = new pthread_t;
}//这样做构造函数对吗？
Thread_Pack::~Thread_Pack()
{
    pthread_join(*thread_c,NULL);
    delete thread_c;
}
//每一个线程都需要执行的函数，用来处理连接后的相关业务
static void *threadpool_thread(void *threadpool)
{
    int this_thread_id = 0;
    ThreadPool* pool = reinterpret_cast<ThreadPool*>(threadpool);
    std::unique_lock<std::mutex>lg(pool->thread_pool_mutex);
    this_thread_id = pool->thread_id_counter;
    pool->thread_id_counter++;
    Task_Function_Arg* tk;
    for(;;)
    {
        // 没有与参数列表匹配的 重载函数 "std::condition_variable::wait" 实例
        if(!lg.owns_lock())
        {
            lg.lock();
        }
        pool->thread_pool_conv.wait(lg,[pool](){
        return (pool->shutdown==true || pool->task_wait_count!=0);
        });
        //如果通知线程关闭，或者有数据需要处理，则继续：
        //std::cout<<"now we have "<<pool->task_wait_count<<" task"<<std::endl;
        if(pool->shutdown)
        {
            break;
        }
        else
        {
            tk = pool->task_queue[pool->head];
            pool->consume_a_task_update_idex();
            lg.unlock();
        }
        // std::cout<<"---------------Thread "<<this_thread_id
        // <<" start consuming a task---------------"<<std::endl;
        (*(tk->function))(tk->arg);
        // std::cout<<"Now we are in the thread, fd is "<<
        // reinterpret_cast<requestData*>(tk->arg)->getFd()<<std::endl;
        // std::cout<<"---------------Thread "<<this_thread_id
        // <<" finish a task---------------"<<std::endl;
    }
    pool->start_thread--;
    //std::cout<<"Thread "<<this_thread_id<<" exit"<<std::endl;
    pthread_exit(NULL);
    return(NULL);
}
int DestroyThreadP(ThreadPool* thr_p)
{
    std::unique_lock<std::mutex>mt(thr_p->thread_pool_mutex);
    if(thr_p==nullptr)
    {
        return THREADPOOL_INVALID;
    }
    if(thr_p->shutdown)
    {
        return THREADPOOL_SHUTDOWN;
    }
    mt.unlock();
    delete thr_p;
    return 1;
}
ThreadPool::ThreadPool(int num_thread,int queue_size_)
{
    queue_size = queue_size_;
    head = 0;tail = 0;
    task_wait_count = 0;
    shutdown = false;
    start_thread = 0;
    // std::cout<<"size of Task_Function_Arg"<<sizeof(Task_Function_Arg)<<std::endl;
    for(int i=0;i<queue_size;i++)
    {
        task_queue.push_back(
        MemoryManager::newElement<Task_Function_Arg>());
        // task_queue.push_back(new Task_Function_Arg());
        // std::cout<<i<<" "<<task_queue[i]<<std::endl;
    }
    for(int i=0;i<num_thread;i++)
    {
        all_thr.push_back(
        MemoryManager::newElement<Thread_Pack>(i));
        // all_thr.push_back(new Thread_Pack(i));
    }
}
void ThreadPool::consume_a_task_update_idex()
{
    now_all_con = now_all_con+1;
    // std::cout<<"NOW finish service is "<<now_all_con<<std::endl;
    head ++;
    task_wait_count --;
    head = head % queue_size;
}
void ThreadPool::add_a_task_update_idex()
{
    tail ++;
    task_wait_count ++;
    tail = tail % queue_size;
}
int ThreadPool::treadpoll_add_task(void (*function)(void*),void* arg)
{
    if(task_wait_count==queue_size)
    {
        //std::cout<<"task_queue is full, we can't add a task."<<std::endl;
        // thread_pool_conv.notify_one();//这里其实应该要notify
        return -1;
    }
    std::unique_lock<std::mutex> unl(thread_pool_mutex);
    task_queue[tail]->function = function;
    task_queue[tail]->arg = arg;
    add_a_task_update_idex();
    unl.unlock();
    thread_pool_conv.notify_one();
    return 1;
}
bool ThreadPool::InitialPool()
{
    for(int i=0;i<THREADPOOL_CAPACITY;i++)
    {
        if(pthread_create(all_thr[i]->thread_c,NULL,threadpool_thread,this)!=0)
        {
            shutdown = true;
            return false;
        }
        start_thread++;       
        //std::cout<<"start_thread "<<start_thread<<std::endl; 
    }
    return true;
}

ThreadPool::~ThreadPool()
{
    std::unique_lock<std::mutex>mt(thread_pool_mutex);
    mt.unlock();
    thread_pool_conv.notify_all();
    for(int i=0;i<task_queue.size();i++)
    {
        MemoryManager::deleteElement<Task_Function_Arg>(task_queue[i]);        
    }
    LOG_INFO<<"All task is freed.\n";
    for(int i=0;i<all_thr.size();i++)
    {
        MemoryManager::deleteElement<Thread_Pack>(all_thr[i]);
        LOG_INFO<<"Thread "<<all_thr[i]->thread_id<<" is freed.\n";
        //线程的join在析构函数中做了
    }
    LOG_INFO<<"ThreadPool is deleted.\n";
    return;
}