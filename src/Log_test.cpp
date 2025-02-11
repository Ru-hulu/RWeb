#include"Logger.hpp"
#include"AsyncLogging.hpp"
#include<time.h>
#include<unistd.h>
#include<pthread.h>
#include<mutex>
AsyncLogging* async=nullptr;
void Cout_(char* a,int b)
{
    async->BuffWriteFunc(a,b);
}
struct T_data
{
    int thread_id;
    int loop_time;
};
std::mutex mx;
int finish_t = 0;
void* ThreadF(void* d_)
{
    T_data* d = (T_data*) d_;
    int loo_t = d->loop_time;
    int loo_id = d->thread_id;
    for(int i=0;i<loo_t;i++)
    {
            usleep(100);
            int t = rand();
            float t_ = float(t)/float(RAND_MAX);
            int m = int(t_/0.34);
            switch (m)
            {
                case 0:
                    LOG_INFO<<"Thread ID is "<<loo_id<<'\n';
                    break;
                case 1:
                    LOG_WARN<<"Thread ID is "<<loo_id<<'\n';
                    break;
                case 2:
                    LOG_ERR<<"Thread ID is "<<loo_id<<'\n';
                    break;
            }
    }
    std::unique_lock<std::mutex>ul(mx);
    finish_t++;
    ul.unlock();
    pthread_exit(NULL);
    return NULL;
}
int main(int argc, char** argv)
{
    Log::set_level(Log::LogLevel::INFO);
    Log::SetCout(Cout_);
    async = new AsyncLogging();
    std::vector<pthread_t*> all_t;
    std::vector<T_data*> all_d;
                    LOG_INFO<<"In main ";
    for(int i=0;i<=3;i++)
    {
        pthread_t* t_t = new pthread_t;
        T_data* t_d = new T_data;
        all_t.push_back(t_t);
        all_d.push_back(t_d);
        t_d->thread_id = i;
        t_d->loop_time = 100000;
        pthread_create(t_t,NULL,ThreadF,(void*)t_d);
    }
    while(finish_t<4)usleep(1000);
    for(int i=0;i<=3;i++)
    {
        delete all_d[i];
        delete all_t[i];
    }
    delete async;
    std::cout<<"TEST success"<<std::endl;
    return 1;
}
