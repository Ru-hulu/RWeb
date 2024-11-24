#include<iostream>
#include <sys/time.h>
#include <mutex>
#include<vector>
#include<unordered_map>
#include"epoll.hpp"
#include"util.hpp"
#include<queue>
#ifndef REQUESTDATA_HPP
#define REQUESTDATA_HPP

const int STATE_PARSE_URI = 1;
const int STATE_PARSE_HEADERS = 2;
const int STATE_RECV_BODY = 3;
const int STATE_ANALYSIS = 4;
const int STATE_FINISH = 5;

const int AGAIN_MAX_TIMES = 200;
const int MAX_BUFF = 4096;

const int PARSE_URI_AGAIN = -1;
const int PARSE_URI_ERROR = -2;
const int PARSE_URI_SUCCESS = 0;

const int PARSE_HEADER_AGAIN = -1;
const int PARSE_HEADER_ERROR = -2;
const int PARSE_HEADER_SUCCESS = 0;

const int METHOD_POST = 1;
const int METHOD_GET = 2;
extern std::mutex mutex_timer_q;
class requestData;
class myTimer;
void myHandlefunc(void* arg);
class requestData
{
    public:
        requestData(int fd_,int epoll_fd_);
        ~requestData();
        void seperateTimer();
        myTimer* mtimer;
        void setFd(int fd_);
        int getFd();
        void setTimer(myTimer* tm_);
        void handlerequest();
        int parse_URI();
        int parse_HEADER();
        void reset_data();
    private:
        int fd;
        int epoll_fd;
        std::string content;//读到请求的内容
        int method;// 从http完整的请求中解析得到的HTTP动作：post 或者 get
        int state_handle;//当前需要处理的状态
        int readagaintime;//有处理信号，但是连续读内容为空的次数
        bool keepalive; // 处理以后是否保持活跃状态
        std::unordered_map<std::string,std::string> header_map;
};

class myTimer
{
    /* data */
    public:
        myTimer(requestData* rqtp_,size_t timeoutms);
        ~myTimer();
        size_t get_expire_time();
        bool isValid();
        void clearRqt();
        size_t expired_time;
    private:
        requestData* rqtp;
        bool deleted;//是否允许删除，就是定时器已经和requetsdata无关联，可以判定为失效        
};
struct timecmp
{
   bool operator()(const myTimer* a,const myTimer* b)const;
};

extern std::priority_queue<myTimer*,std::vector<myTimer*>,timecmp>Prio_timer_queue;
#endif