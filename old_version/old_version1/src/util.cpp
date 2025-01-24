#include"util.hpp"
int socketNonBlock(int fd)
{
    int flag = 0;
    flag = fcntl(fd,F_GETFL,0);//获得文件描述符的状态
    if(flag==-1)
    {
        return -1;
    }
    flag |= O_NONBLOCK;//设置文件描述符为非阻塞状态
    if(fcntl(fd,F_SETFL,flag)==-1)
    return -1;
    return 0;
}

//当向一个已经关闭的管道、套接字等写入数据时，系统会发送 SIGPIPE 信号，会终止程序。
//这里IGNORE信号。
bool handle_sigpipe()
{
    struct sigaction sga;
    memset(&sga,'\0',sizeof(sga));
    sga.sa_flags = 0;
    sga.sa_handler = SIG_IGN;
    if(sigaction(SIGPIPE,&sga,NULL)!=0)
    {
        return false;
    }
    return true;
}
//这里给的n是最大可能的读取长度，实际长度可能小于n
ssize_t read_n(int fd_,void* bf,size_t n)
{
    size_t nleft = n;
    size_t nread = 0;
    char* np = reinterpret_cast<char*>(bf);
    while(nleft>0)
    {
        ssize_t r;
        r = read(fd_,np,nleft);
        if(r<0)
        {   //这说明已经发生错误了
            if(errno == EAGAIN)//资源不可用
            {
                return nread;
            }
            else if(errno==EINTR)//读取被中断，可以继续读。
            {
                r = 0;
            }
            else 
            {
                return -1;
            }
        }
        else if(r==0)
        {
            break;
        }
        nleft -= r;
        nread += r;
        np += r;
    }
    return nread;
}

// 系统资源如内存、文件描述符等都是有限的。
// 当系统资源紧张时，write系统调用可能无法一次性完成所有数据的写入。
// 比如，在网络编程中，如果网络缓冲区已满，write调用可能只能写入部分数据，剩余数据需要等待网络缓冲区有空间时再写入。
ssize_t write_n(int fd_,void* bf,size_t n)
{
    size_t swrite = 0 ;
    char* np = reinterpret_cast<char*>(bf);
    size_t lwrite = n;
    while(lwrite>0)
    {
        size_t nw = write(fd_,bf,lwrite);
        if(nw<0)
        {
            if(errno == EAGAIN || errno == EINTR)//资源不可用 或者 信号被中断
            {
                nw = 0;
            }
            else
                return -1;
        }
        else if(nw == 0)
        {
            return swrite;
        }
        bf += nw;
        swrite += nw;
        lwrite -= nw;
    }
    return swrite;
}