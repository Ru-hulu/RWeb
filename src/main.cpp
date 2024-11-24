#include"epoll.hpp"
#include "requestData.hpp"
#include "ThreadPool.hpp"
#include "util.hpp"
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<queue>
#include<vector>
std::mutex mutex_timer_q;
bool timecmp::operator()(const myTimer* a,const myTimer* b)const
{
    return ((a)->expired_time >= (b)->expired_time); 
}
std::priority_queue<myTimer*,std::vector<myTimer*>,timecmp>Prio_timer_queue;

int socket_bind_listen(int port)
{
    if(!(port>=1024 && port<=65536))
    return -1;
    int listen_fd = socket(AF_INET,SOCK_STREAM,0);//IPV4+tcp
    if(listen_fd<0) return -1;
    int opttime = 1;
    //SOL_SOCKET代表用于套接字本身的通用选项，而非某个协议(TCP,UDP)的配置
    //SO_REUSEADDR选项主要用于允许一个套接字绑定到一个处于TIME - WAIT状态的本地地址和端口上。
    //在正常情况下，当一个 TCP 连接关闭后，本地端的套接字会进入TIME - WAIT状态
    //这个状态会持续一段时间（通常是 2MSL，其中 MSL 是最长报文段寿命）。
    //在这个期间，如果没有设置SO_REUSEADDR选项，其他进程将无法绑定到该套接字使用的本地地址和端口。
    //设置了这个选项后，就可以绕过这个限制，使得服务器能够快速重启并绑定到之前使用的地址和端口，提高服务器的可用性和灵活性。
    //opttime=1表示允许套接字绑定到TIME-WAIT端口上
    if(setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&opttime,sizeof(opttime))==-1)
    {
        return -1;
    }
    struct sockaddr_in sock_in;
    bzero((char*)(&sock_in),sizeof(sock_in));
    sock_in.sin_family = AF_INET;
    sock_in.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_in.sin_port = htons(uint16_t(port));
    if(bind(listen_fd,(struct sockaddr*)(&sock_in),sizeof(sock_in))==-1)
    {
        return -1;
    }
    // INADDR_ANY：服务器将监听来自任何可用的本地 IPv4 地址的连接请求。
    // htonl/htons ：字节序转换函数，用于将主机字节序（Host Byte Order）转换为网络字节序（Network Byte Order）。
    // 在网络通信中，不同的计算机系统可能采用不同的字节序（大端序或小端序）来存储多字节的数据
    // 而网络协议规定了统一的网络字节序，所以在设置 IP 地址等网络相关数据时
    // 需要进行这样的字节序转换操作，以确保数据在网络中的正确传输和理解。
    if(listen(listen_fd,LISTENQ)==-1)
    {
        return -1;
        //设置套接字进入监听状态，告知操作系统内核这个套接字可以接受客户端的连接请求，并且初始化一个连接请求队列
        //当客户端发起连接请求时，操作系统会将这些请求放入这个队列中。        
        //后续一般有一个accept调用，是阻塞的。
        //accept从监听队列中取出一个连接请求，并返回一个新的套接字文件描述符，用于和该客户端进行通信。
    }
    return listen_fd;
}

void HandleExpire()
{
    std::lock_guard<std::mutex> lckg(mutex_timer_q);
    while(Prio_timer_queue.size())
    {
        myTimer* tt = Prio_timer_queue.top();
        if(tt->isValid())
        {
            break;
        }
        else
        {
            Prio_timer_queue.pop();
            delete tt;
        }
    }
}
const int PORT = 8080;
const std::string PATH = "/";
const int THREAD_NUM = 4;
const int QSIZE = 1024;
const uint32_t TIMEOUT = 500;
const uint32_t MAXQ = 500;
void AcceptNewConnection(int listen_fd,int epoll_fd,ThreadPool* tp_)
{
    sockaddr_in sk;
    bzero(&sk,sizeof(sk));
    socklen_t skl = 0;
    int acc_fd = -1;
    //当epoll检测到监听套接字上有可读事件时，这可能意味着有一个或多个客户端正在尝试连接。
    while((acc_fd = accept(listen_fd,(sockaddr*)(&sk),&skl))>0)
    {
        if(socketNonBlock(acc_fd)<0)
        {
            std::cout<<"set non block failed!!"<<std::endl;
            continue;
        }
        requestData* rqt_ = new requestData(acc_fd,epoll_fd);
        uint32_t ev_id = EPOLLIN|EPOLLET|EPOLLONESHOT;
        int rt = epoll_add(epoll_fd,reinterpret_cast<void*>(rqt_),acc_fd,ev_id);
        if(rt<0)
        {
            delete rqt_;
            continue;
        }
        myTimer* tm = new myTimer(rqt_,TIMEOUT);
        rqt_->setTimer(tm);
        std::unique_lock<std::mutex>ukl(tp_->thread_pool_mutex);
        Prio_timer_queue.push(tm);
        ukl.unlock();
    }

    // sockfd：已经绑定（通过 bind 函数）并监听的套接字文件描述符。
    // addr：成功接受一个客户端连接时，客户端的地址信息填充到这个结构体中。(struct sockaddr_in)
    // addrlen：这是一个指向 socklen_t 类型变量的指针。在 accept 函数返回后，它会被更新为实际填充的客户端地址结构体的长度。
    // 其初始值应该是 client_addr 结构体的初始长度。
}
void Handle_Events(int epoll_fd,int listen_fd,int even_size,ThreadPool* tp)
{
    for(int i=0;i<even_size;i++)
    {
        epoll_event e = events_get[i];
        requestData* rqt_ = reinterpret_cast<requestData*>(e.data.ptr);
        int happen_fd = rqt_->getFd();
        if(happen_fd == listen_fd)
        {
            AcceptNewConnection(listen_fd,epoll_fd,tp);
        }
        else if((e.events & EPOLLHUP)||(e.events & EPOLLERR) || !(e.events & EPOLLIN))
        {
            delete rqt_;//epoll会删除这个链接
            continue;
        }        //有一方挂起、连接出错、触发事件，但是没有可读数据的时候。
        else//正常触发,需要找一个线程来处理
        {
            rqt_->seperateTimer();
            tp->treadpoll_add_task(myHandlefunc,reinterpret_cast<void*>(rqt_));
        }
    }
}
int main(int argc, char** argv)
{
    int epoll_fd = epoll_init();
    int listen_fd = socket_bind_listen(PORT);
    if(listen_fd<0)
    {
        std::cout<<"socket_bind_listen failed!!!"<<std::endl;
        return 1;
    }
    if(socketNonBlock(listen_fd)<0)
    {
        std::cout<<"socketNonBlock failed!!!"<<std::endl;
        return 1;
    }
    requestData* rqt_data = new requestData(epoll_fd,listen_fd);
    rqt_data->setFd(listen_fd);
    uint32_t eve_id = EPOLLIN|EPOLLET;
    epoll_add(epoll_fd,reinterpret_cast<void*>(rqt_data),listen_fd,eve_id);
    ThreadPool tp(THREAD_NUM,QSIZE);
    if(tp.InitialPool()==false)
    {
        tp.~ThreadPool();
        std::cout<<"initial failed"<<std::endl;
        return 1;
    }//以下tp不可能出现shutdown的情况。

    while(true)
    {
        int even_size = my_epoll_wait(epoll_fd,events_get,TIMEOUT,MAXQ);
        if(even_size<=0)continue;
        Handle_Events(epoll_fd,listen_fd,even_size,&tp);
        HandleExpire();
        //拿到所有的触发事件
        //判定如果是连接事件，处理新的连接
        //如果不是连接事件，就用线程池分配给线程处理
    }
    socket_bind_listen(8080);
    return 1;
}