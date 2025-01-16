#ifndef REQUESTDATA
#define REQUESTDATA
#include <string>
#include <unordered_map>

const int STATE_PARSE_URI = 1;
const int STATE_PARSE_HEADERS = 2;
const int STATE_RECV_BODY = 3;
const int STATE_ANALYSIS = 4;
const int STATE_FINISH = 5;

const int MAX_BUFF = 4096;

// 有请求出现但是读不到数据,可能是Request Aborted,
// 或者来自网络的数据没有达到等原因,
// 对这样的请求尝试超过一定的次数就抛弃
const int AGAIN_MAX_TIMES = 200;

const int PARSE_URI_AGAIN = -1;
const int PARSE_URI_ERROR = -2;
const int PARSE_URI_SUCCESS = 0;

const int PARSE_HEADER_AGAIN = -1;
const int PARSE_HEADER_ERROR = -2;
const int PARSE_HEADER_SUCCESS = 0;

const int ANALYSIS_ERROR = -2;
const int ANALYSIS_SUCCESS = 0;

const int METHOD_POST = 1;
const int METHOD_GET = 2;
const int HTTP_10 = 1;
const int HTTP_11 = 2;

const int EPOLL_WAIT_TIME = 500;

class MimeType
{
private:
    static pthread_mutex_t lock;
    static std::unordered_map<std::string, std::string> mime;
    MimeType();
    MimeType(const MimeType &m);
public:
    static std::string getMime(const std::string &suffix);
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

struct mytimer;
struct requestData;

struct requestData
{
private:
    int againTimes;
    std::string path;
    int fd;//猜测：一个requestData维护一个客户端连接。
    int epollfd;//多个requestData对应一个epollfd
    // content的内容用完就清
    std::string content; // http完整的请求的内容
    int method;// 从http完整的请求中解析得到的HTTP动作：post 或者 get
    int HTTPversion; //从http完整的请求中解析得到的版本
    std::string file_name;//http完整的请求 URI 中解析得到的文件名称
    int now_read_pos;
    int state;
    int h_state;//行的状态
    bool isfinish;
    bool keep_alive;
    std::unordered_map<std::string, std::string> headers;//对http请求头的解析结果
    mytimer *timer;

private:
    int parse_URI();
    int parse_Headers();
    int analysisRequest();

public:

    requestData();
    requestData(int _epollfd, int _fd, std::string _path);
    ~requestData();
    void addTimer(mytimer *mtimer);
    void reset();
    void seperateTimer();
    int getFd();
    void setFd(int _fd);
    void handleRequest();
    void handleError(int fd, int err_num, std::string short_msg);
};

struct mytimer
{
    bool deleted;
    size_t expired_time;
    requestData *request_data;

    mytimer(requestData *_request_data, int timeout);
    ~mytimer();
    bool isvalid();
    void clearReq();
    void setDeleted();
    bool isDeleted() const;
    size_t getExpTime() const;
};

struct timerCmp
{
    bool operator()(const mytimer *a, const mytimer *b) const;
};
#endif