#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <utility>
#include <mutex>
#include <unistd.h>
#include <unordered_map>
#include "MemoryPool.hpp"
// 测试用例1：仅考虑内存池非new非配的空间
// int pool_test_low = 8;
// int pool_test_hig = 512;
// int loopIterations = 100000;
// 测试用例2：仅考虑内存池new非配的空间
// int pool_test_low = 516;
// int pool_test_hig = 1024;
// int loopIterations = 100000;

std::mutex mx;
int finish_flag = 0;
int pool_test_low = 516;
int pool_test_hig = 1024;
int loopIterations = 10;

template <int N>
class Person {
public:
    int d[N];
    Person() {
        for (int i = 0; i < N; ++i) {
            d[i] = i;
        }
    }
};
using CreatePersonFunc = void* (*)();
CreatePersonFunc createFuncs[520];
template <int N>
void* createPerson() {
    // return (void*) MemoryManager::newElement<Person<N>>();
    return ((void*) (new Person<N>()));
}
template <int N>
struct InitializeCreateFuncs 
{
    static void init() 
    {
        if(N > 1)
        {
            InitializeCreateFuncs<N - 1>::init();
        }
        createFuncs[N - 1] = createPerson<N>;
    }
};
template <>
struct InitializeCreateFuncs<1> 
{
    static void init() 
    {
        createFuncs[0] = createPerson<1>;
    }
};

using DeletePersonFunc = void (*)(void*);
DeletePersonFunc deleteFuncs[520];
template <int N>
void deletePerson(void* p_) 
{
    // MemoryManager::deleteElement<Person<N>>((Person<N>*)p_);
    delete (Person<N>*)p_;
}
template <int N>
struct InitializeDeleteFuncs 
{
    static void init() 
    {
        if(N > 1)
        {
            InitializeDeleteFuncs<N - 1>::init();
        }
        deleteFuncs[N - 1] = deletePerson<N>;
    }
};
template <>
struct InitializeDeleteFuncs<1> 
{
    static void init() 
    {
        deleteFuncs[0] = deletePerson<1>;
    }
};
void* RandomThreadFunc(void* arg_)
{
    int d = rand();
    int by = float(d)/float(RAND_MAX) * float(7); 
    by = std::min(by,6);
    std::unordered_map<int,void*> all_p;
        int i_tes_low = pool_test_low / sizeof(int);
        int i_tes_hig = pool_test_hig / sizeof(int);


    for (int i = 0; i < loopIterations; ++i) 
    {
        int d = std::rand();
        int by = static_cast<int>(static_cast<float>(d) / RAND_MAX * float(i_tes_hig-i_tes_low)
         + float(i_tes_low));
        by = 140;
        if (by >=i_tes_low && by <= i_tes_hig) 
        {
            void* personPtr = createFuncs[by - 1]();
            all_p.insert({by,personPtr});
        }
        // if(by%3==0)
        // {
        //     deleteFuncs[all_p.begin()->first-1](all_p.begin()->second);
        //     all_p.erase(all_p.begin()); 
        // }
        usleep(30);
    }   
    for(auto& a:all_p)
    {
        deleteFuncs[a.first-1](a.second);
    }
    std::unique_lock<std::mutex> uk(mx);
    finish_flag++;
    uk.unlock();
    pthread_exit(NULL);
    return(NULL);
}
void* ThreadFunc(void* arg_)
{
    std::unordered_map<int,void*> all_p;
    for (int i = 1; i <= 520; ++i) 
    {
        void* personPtr = createFuncs[i - 1]();
        all_p.insert({i,personPtr});
        if(i%3==0)
        {
            deleteFuncs[all_p.begin()->first-1](all_p.begin()->second);
            all_p.erase(all_p.begin()); 
        }
        usleep(30);
    }   
    for(auto& a:all_p)
    {
        deleteFuncs[a.first-1](a.second);
    }
    std::unique_lock<std::mutex> uk(mx);
    finish_flag++;
    uk.unlock();
    pthread_exit(NULL);
    return(NULL);
}

int main(int argc, char** argv) 
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    InitializeCreateFuncs<520>::init();
    InitializeDeleteFuncs<520>::init();
    MemoryManager::Initallpool();//初始化内存池
    // void* personPtr = createFuncs[23]();   
    // deleteFuncs[23](personPtr);
    std::vector<pthread_t*>all_thread;
    for(int i=0;i<4;i++)
    {
        pthread_t* t_ = new pthread_t;
        all_thread.push_back(t_);
        // pthread_create(t_,NULL,ThreadFunc,nullptr);
        pthread_create(t_,NULL,RandomThreadFunc,nullptr);
    }
    while(finish_flag!=4)usleep(1000);
    for(int i=0;i<4;i++)delete all_thread[i];
    std::cout<<"All Thread Finished, MemoryPool Test Success"<<std::endl;
    return 0;
}

// 在提供的代码中，InitializeCreateFuncs 的主模板使用运行时的 if (N > 1) 来控制递归调用。
// 然而，模板实例化发生在编译时，即使条件为假，编译器仍需检查所有代码路径的语法有效性，
// 包括递归调用 InitializeCreateFuncs<N-1>::init()。当没有特化版本时，这会导致模板递归无限进行：
// 实例化触发递归：当实例化 InitializeCreateFuncs<1> 时，编译器必须解析 InitializeCreateFuncs<0>（尽管 if (1 > 1) 为假），这会触发 InitializeCreateFuncs<0> 的实例化。
// 无效的数组索引：InitializeCreateFuncs<0> 的实例化会导致 createFuncs[N-1] 变成 createFuncs[-1]，这是非法索引，导致编译错误。
// 特化版本的作用：显式特化 InitializeCreateFuncs<1> 提供了递归终止条件，避免实例化 InitializeCreateFuncs<0>，从而阻止无效索引的产生。没有特化时，编译器被迫处理 N=0 的情况，导致错误。
// 结论：模板递归需要显式的编译时终止条件（如特化），运行时的条件判断无法阻止模板实例化的递归，从而导致编译错误。