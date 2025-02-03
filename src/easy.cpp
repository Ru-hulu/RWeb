#define BUFFER_SIZE 1024
#include<sys/epoll.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<errno.h>
#include<iostream>
struct temp_struct
{
    int fd = -1;
    int d = 1;
    temp_struct(int fd_)
    {
        fd = fd_;
    }
};
epoll_event* all_eve;
int main() {
int listen_fd = socket(AF_INET,SOCK_STREAM,0);
std::cout<<listen_fd<<std::endl;
int opt = 1;
std::cout<<setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR|SO_REUSEPORT,&opt,sizeof(opt));
struct sockaddr_in sockaddr_d;
sockaddr_d.sin_family = AF_INET;
sockaddr_d.sin_addr.s_addr = INADDR_ANY;
sockaddr_d.sin_port = htons(8888);
std::cout<<bind(listen_fd,(struct sockaddr*)(&sockaddr_d),sizeof(sockaddr_d));
if(listen(listen_fd,1024)==-1)
{
    return -1;
    //设置套接字进入监听状态，告知操作系统内核这个套接字可以接受客户端的连接请求，并且初始化一个连接请求队列
    //当客户端发起连接请求时，操作系统会将这些请求放入这个队列中。        
    //后续一般有一个accept调用，是阻塞的。
    //accept从监听队列中取出一个连接请求，并返回一个新的套接字文件描述符，用于和该客户端进行通信。
}
else std::cout<<"listen";

int epoll_fd = epoll_create(100);
std::cout<<epoll_fd;
epoll_event evd;
evd.data.ptr = new temp_struct(listen_fd);
evd.events = EPOLLIN|EPOLLET;
epoll_ctl(epoll_fd,EPOLL_CTL_ADD,listen_fd,&evd);
all_eve = new epoll_event[10];
char buff[1024];
size_t coutt = 0;
while(1)
{
    int evt = epoll_wait(epoll_fd,all_eve,10,500);    
    if(evt==0)
    {
        std::cout<<"no even"<<std::endl;
        continue;
    }
    else if(evt<0)
    {
        std::cout<<"error"<<std::endl;
    }
    else
    {
        for(int i=0;i<evt;i++)
        {
            int even_fd = reinterpret_cast<temp_struct*>(all_eve[i].data.ptr)->fd;
            if(even_fd==listen_fd)
            {
                sockaddr_in sock_a;
                socklen_t lent;
                int new_fd = accept(listen_fd,(sockaddr*)(&sock_a),&lent);
                epoll_event evdn;
                evdn.data.ptr = new temp_struct(new_fd);
                evdn.events = EPOLLIN|EPOLLET;
                epoll_ctl(epoll_fd,EPOLL_CTL_ADD,new_fd,&evdn);
            }
            else
            {
                coutt++;
                ssize_t t = read(even_fd,buff,1024);
                if(errno==0 && t==0)
                std::cout<<"fd is closed by client "<<coutt<<std::endl;
                epoll_event evdn;
                evdn.data.ptr = all_eve[i].data.ptr;
                evdn.events = EPOLLIN|EPOLLET;
                // //套接字缓冲区有数据可读，或者对端关闭连接
                // //文件描述符的状态发生变化时触发
                // //在一次事件触发后，该文件描述符上的事件将被自动移除，
                // //后续即使文件描述符的状态再次发生符合事件条件的变化，epoll 也不会再触发该文件描述符的事件通知，除非重新使用 epoll_ctl 函数对其进行设置。
                epoll_ctl(epoll_fd,EPOLL_CTL_MOD,even_fd,&evdn);
            }
        }
    }
}
return 0;
}