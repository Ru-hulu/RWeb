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

const int HTTP_10 = 1;
const int HTTP_11 = 2;
const int ANALYSIS_ERROR = -2;
const int ANALYSIS_SUCCESS = 0;
extern std::mutex mutex_timer_q;
extern std::mutex f_n_mtx;
extern size_t f_p_n;
class requestData;
class myTimer;
void myHandlefunc(void* arg);
extern std::unordered_map<std::string,std::string> mime;
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
        int analysisRequest();
        void reset_data();
        std::string getMime(std::string t);
        void handleError(int fd, int err_num, std::string short_msg);
    private:
        int fd;
        int epoll_fd;
        std::string file_name;
        std::string content;//读到请求的内容
        int method;// 从http完整的请求中解析得到的HTTP动作：post 或者 get
        int HTTPversion; //从http完整的请求中解析得到的版本
        int state_handle;//当前需要处理的状态
        int readagaintime;//有处理信号，但是连续读内容为空的次数
        bool keepalive; // 处理以后是否保持活跃状态
        int h_state;//行的状态
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
        int get_fd();
        size_t expired_time;
    private:
        requestData* rqtp;
        bool deleted;//是否允许删除，就是定时器已经和requetsdata无关联，可以判定为失效        
};
struct timecmp
{
   bool operator()(const myTimer* a,const myTimer* b)const;
};
enum HeadersState
{
    h_start = 0,
    h_key,//准备进行键的解读
    h_colon,//刚刚解析完头部信息的键，现在遇到了:
    h_spaces_after_colon,//键解读后的空格
    h_value,//准备进行值的解读
    h_CR,//值后面的回车\r
    h_LF,//一行头部信息的结束
    h_end_CR,//整个头部信息结束前的最后一个回车符，进入此状态等待最后一个换行符确认。
    h_end_LF//头部信息的解读完成
};
extern std::priority_queue<myTimer*,std::vector<myTimer*>,timecmp>Prio_timer_queue;

#endif