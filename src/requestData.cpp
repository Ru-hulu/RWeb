#include"requestData.hpp"
#include<unistd.h>
myTimer::myTimer(requestData* rqtp_,size_t timeoutms):rqtp(rqtp_)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    expired_time = tv.tv_sec*1000 + tv.tv_usec/1000 + timeoutms;
    return;
}
myTimer::~myTimer()
{
    //析沟函数中需要释放requestData;
    if(rqtp!=nullptr)
    {
        delete rqtp;
    }    
    return;
}
size_t myTimer::get_expire_time()
{
    return expired_time;
}
void myTimer::clearRqt()
{
    rqtp = nullptr;
}

bool myTimer::isValid()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    size_t nowt = tv.tv_sec*1000 + tv.tv_usec/1000;
    if(nowt>expired_time || rqtp==nullptr)
    {
        return false;
    }
    return true;
}
requestData::requestData(int fd_,int epoll_fd_)
{
    fd = fd_;
    epoll_fd = epoll_fd_;
    state_handle = STATE_PARSE_URI;
    keepalive = false;
    readagaintime = 0;
}
requestData::~requestData()
{
    int ev_id = EPOLLIN|EPOLLET|EPOLLONESHOT;
    epoll_del(epoll_fd,fd,ev_id);//删除epoll_fd 与 fd的关联
    close(fd);//关闭文件描述符
    if(mtimer!=nullptr)
    {
        seperateTimer();
    }
}
void requestData::setTimer(myTimer* tm_)
{
    mtimer = tm_;
}
void requestData::seperateTimer()
{
    mtimer->clearRqt();
    mtimer = nullptr;
}
void requestData::setFd(int fd_)
{
    fd = fd_;
}

int requestData::getFd()
{
    return fd;
}

void myHandlefunc(void* arg)
{
    requestData* this_d = reinterpret_cast<requestData*>(arg);
    this_d->handlerequest();
}

//1、当fd上有可读数据时，数据不能够保证是完整的http请求
//每次读取可能都只能读取到部分的数据，然后依据部分的数据拼凑完整的请求。
//问题：既然我们设计的时候采用的是边沿触发EPOLLET,那么我们如何保证每次数据包都能够被epoll_wait发现呢？
//我们在每次read的时候，都将缓冲区中的数据读完，即一直读取到read_n返回0为止。这样每次的数据到达fd的时候，就都能够触发可读事件了。
//EAGAIN:资源不可用，这里是提示没有数据可以读。
void requestData::reset_data()
{
    content.clear();
    keepalive = false;
    method = -1;
    readagaintime = 0;
    keepalive = false;
    header_map.clear();
}
int requestData::parse_URI()
{   
    return 1;
}
int requestData::parse_HEADER()
{   
    return 1;
}
void requestData::handlerequest()
{
    bool isError = false;
    char buf[MAX_BUFF];
    while(true)
    {
            size_t r = read_n(fd,buf,MAX_BUFF);
            if(r<0)
            {
                isError = true;
                break;
                //如果读的过程中出错了，那么直接删除这个连接
            }
            else if(errno==EAGAIN&&r==0)
            {
                if(readagaintime>=AGAIN_MAX_TIMES)
                {
                    //当前的连接出错，直接作废。
                    isError = true;
                }
                else
                {
                    //等待下一次触发,content不清空
                    readagaintime++;
                }
                break;
            }
            //如果读到了数据：
            std::string now_read(buf,buf+r);
            content += now_read;
            if(state_handle==STATE_PARSE_URI)
            {
                int st = parse_URI();
                if(st==PARSE_URI_AGAIN)
                {//提示当前的数据还不够完整，需要再读数据
                    break;
                }
                else if(st==PARSE_URI_ERROR)
                {
                    isError = true;
                    break;
                }
                else if(st == PARSE_URI_SUCCESS)state_handle = STATE_PARSE_HEADERS;
            }//URI已经解析成功
            if(state_handle==STATE_PARSE_HEADERS)
            {
                int st = parse_HEADER();
                if(st == PARSE_HEADER_AGAIN)
                {//提示当前的数据还不够完整，需要再读数据
                    break;
                }
                else if(st == PARSE_HEADER_ERROR)
                {
                    isError = true;
                    break;
                }
                if(method==METHOD_GET)
                {
                    state_handle = STATE_ANALYSIS;
                }
                else
                {//这里等待请求体长度
                    state_handle = STATE_RECV_BODY;
                }
            }
            if(state_handle == STATE_RECV_BODY)
            {
                if(header_map.find("Content-Length")==header_map.end())
                {
                    isError = true;
                    break;
                }
                int con_size = std::stoi(header_map["Content-Length"]);
                if(content.size()<con_size)
                continue;
                state_handle = STATE_ANALYSIS;
            }
            if(state_handle == STATE_ANALYSIS)
            {
                
            }
        }
        if(isError)
        {
            delete this;
            return;
        }
        if(state_handle==STATE_FINISH)
        {
            if(keepalive)
            {
                reset_data();
            }
            else
            {
                delete this;
                return;
            }
        }


        myTimer* tm = new myTimer(this,500);
        mtimer = tm;
        std::unique_lock<std::mutex>(mutex_timer_q);
        Prio_timer_queue.push(tm);
        //上述的功能应该已经处理好了，接下来应该是连接保留的工作
        int ev_id = EPOLLIN|EPOLLONESHOT|EPOLLET;
        epoll_add(epoll_fd,this,fd,ev_id);
}