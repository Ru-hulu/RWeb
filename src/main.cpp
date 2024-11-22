#include"epoll.hpp"
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
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
int main(int argc, char** argv)
{
    socket_bind_listen(8080);
    return 1;
}