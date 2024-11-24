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
ssize_t read_n(int fd_,void* bf,size_t n)
{
    size_t nleft = n;
    size_t nread = 0;
    char* np = reinterpret_cast<char*>(bf);
    while(nleft>0)
    {
        size_t r;
        r = read(fd_,np,nleft);
        if(r<0)
        {   //这说明已经发生错误了
            if(errno == EAGAIN)
            {
                return nread;
            }
            else if(errno==EINTR)
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