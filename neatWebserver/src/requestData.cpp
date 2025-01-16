#include "requestData.h"
#include "util.h"
#include "epoll.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/time.h>
#include <unordered_map>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <queue>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
using namespace cv;

//test
#include <iostream>
using namespace std;
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MimeType::lock = PTHREAD_MUTEX_INITIALIZER;
std::unordered_map<std::string, std::string> MimeType::mime;

std::string MimeType::getMime(const std::string &suffix)
{
    if (mime.size() == 0)
    {
        pthread_mutex_lock(&lock);
        if (mime.size() == 0)
        {
            mime[".html"] = "text/html";
            mime[".avi"] = "video/x-msvideo";
            mime[".bmp"] = "image/bmp";
            mime[".c"] = "text/plain";
            mime[".doc"] = "application/msword";
            mime[".gif"] = "image/gif";
            mime[".gz"] = "application/x-gzip";
            mime[".htm"] = "text/html";
            mime[".ico"] = "application/x-ico";
            mime[".jpg"] = "image/jpeg";
            mime[".png"] = "image/png";
            mime[".txt"] = "text/plain";
            mime[".mp3"] = "audio/mp3";
            mime["default"] = "/home/r/dir.txt";
            // mime["default"] = "text/html";
        }
        pthread_mutex_unlock(&lock);
    }
    if (mime.find(suffix) == mime.end())
        return mime["default"];
    else
        return mime[suffix];
}


priority_queue<mytimer*, deque<mytimer*>, timerCmp> myTimerQueue;

requestData::requestData(): 
    now_read_pos(0), state(STATE_PARSE_URI), h_state(h_start), 
    keep_alive(false), againTimes(0), timer(NULL)
{
    cout << "requestData constructed !" << endl;
}

requestData::requestData(int _epollfd, int _fd, std::string _path):
    now_read_pos(0), state(STATE_PARSE_URI), h_state(h_start), 
    keep_alive(false), againTimes(0), timer(NULL),
    path(_path), fd(_fd), epollfd(_epollfd)
{}

requestData::~requestData()
{
    cout << "~requestData()" << endl;
    struct epoll_event ev;
    // 超时的一定都是读请求，没有"被动"写。
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    ev.data.ptr = (void*)this;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
    if (timer != NULL)
    {
        timer->clearReq();
        timer = NULL;
    }
    close(fd);
}

void requestData::addTimer(mytimer *mtimer)
{
    if (timer == NULL)
        timer = mtimer;
}

int requestData::getFd()
{
    return fd;
}
void requestData::setFd(int _fd)
{
    fd = _fd;
}

void requestData::reset()
{
    againTimes = 0;
    content.clear();
    file_name.clear();
    path.clear();
    now_read_pos = 0;
    state = STATE_PARSE_URI;
    h_state = h_start;
    headers.clear();
    keep_alive = false;
}

void requestData::seperateTimer()
{
    if (timer)
    {
        timer->clearReq();
        timer = NULL;
    }
}

//epoll_wait 发现当前fd有请求了，这个timer就会更新
void requestData::handleRequest()
{
    char buff[MAX_BUFF];
    bool isError = false;
    while (true)
    {
        int read_num = readn(fd, buff, MAX_BUFF);
        if (read_num < 0)
        {
            perror("1");
            std::cout<<"read_num<0 !!"<<std::endl;
            isError = true;
            break;
        }
        else if (read_num == 0)
        {
            // 有请求出现但是读不到数据，可能是Request Aborted，或者来自网络的数据没有达到等原因
            perror("read_num == 0");

            if (errno == EAGAIN)
            {
                std::cout<<"EAGAIN !! "<<againTimes<<std::endl;
                if (againTimes > AGAIN_MAX_TIMES)
                    isError = true;
                else
                    ++againTimes;
            }
            else if (errno != 0)
                isError = true;
            break;
        }
        string now_read(buff, buff + read_num);//用buff构造now_read字符串
        content += now_read;

        if (state == STATE_PARSE_URI)
        {
            std::cout<<"now STATE_PARSE_URI"<<std::endl;
            int flag = this->parse_URI();
            if (flag == PARSE_URI_AGAIN)
            {
                break;
            }
            else if (flag == PARSE_URI_ERROR)
            {
                perror("2");
                isError = true;
                break;
            }
        }
        if (state == STATE_PARSE_HEADERS)
        {
            std::cout<<"now STATE_PARSE_HEADERS"<<std::endl;
            int flag = this->parse_Headers();
            if (flag == PARSE_HEADER_AGAIN)
            {  
                break;
            }
            else if (flag == PARSE_HEADER_ERROR)
            {
                perror("3");
                isError = true;
                break;
            }
            if(method == METHOD_POST)
            {
                state = STATE_RECV_BODY;
                for(auto ittt:headers)
                std::cout<<ittt.first<<" "<<ittt.second<<std::endl;
            }
            else 
            {
                state = STATE_ANALYSIS;
            }
        }
        if (state == STATE_RECV_BODY)//POST情况下有请求体
        {
            // std::cout<<"now STATE_RECV_BODY the content is "<<std::endl;
            // std::cout<<content<<"123"<<std::endl;
            int content_length = -1;
            if (headers.find("Content-Length") != headers.end())
            {   //请求体的长度
                content_length = stoi(headers["Content-Length"]);
            }
            else
            {
                isError = true;
                break;
            }
            if (content.size() < content_length)
            {
                std::cout<<"getting data: "<<content.size()<<" / "<<content_length<<std::endl;
                continue;            
            }
            state = STATE_ANALYSIS;
        }
        if (state == STATE_ANALYSIS)
        {
            std::cout<<"now STATE_ANALYSIS"<<std::endl;            
            int flag = this->analysisRequest();
            if (flag == ANALYSIS_SUCCESS)
            {

                state = STATE_FINISH;
                break;
            }
            else
            {
                isError = true;
                break;
            }
        }
    }

    if (isError)
    {
        delete this;
        return;
    }
    // 加入epoll继续
    if (state == STATE_FINISH)
    {
        if (keep_alive)
        {
            printf("ok\n");
            this->reset();
        }
        else
        {
            delete this;
            return;
        }
    }
    // 一定要先加时间信息，否则可能会出现刚加进去，下个in触发来了，然后分离失败后，又加入队列，最后超时被删，然后正在线程中进行的任务出错，double free错误。
    // 新增时间信息
    pthread_mutex_lock(&qlock);
    mytimer *mtimer = new mytimer(this, 500);
    timer = mtimer;
    myTimerQueue.push(mtimer);
    pthread_mutex_unlock(&qlock);

    __uint32_t _epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;
    int ret = epoll_mod(epollfd, fd, static_cast<void*>(this), _epo_event);
    // EPOLLONESHOT：这个标志表示一次性触发。
    // 当设置了 EPOLLONESHOT 后，一旦对应的文件描述符上发生了被监测的事件（比如有数据可读，这里因为同时设置了 EPOLLIN）
    // 那么在处理完这次事件后，该文件描述符的事件监测将会被自动关闭。
    // 如果后续还需要继续监测该文件描述符的事件，就需要重新设置其事件监测状态。
    // 这种设置在一些场景下很有用，比如在多线程处理同一个文件描述符的事件时，可以避免不同线程同时处理同一个事件导致的混乱，
    // 每个线程处理完一次事件后，其他线程再处理就需要重新设置事件监测。
    // EPOLLIN 当前套接字已经准备好读数据了
    // EPOLLET 边沿触发

    if (ret < 0)
    {
        // 返回错误处理
        delete this;
        return;
    }
}


// 该函数 requestData::parse_URI() 的主要目的是对(content):一个包含 HTTP 请求相关内容的字符串进行解析
// 从中提取出重要的信息，如请求方法（GET 或 POST）、请求的文件名、HTTP 版本号等，并根据解析的情况
// 更新请求数据对象的相关状态。如果解析过程中出现各种不符合预期的情况，函数会返回相应的错误码或表示
// 需要再次尝试解析的码值。
int requestData::parse_URI()
{
    string &str = content;
    // 读到完整的请求行再开始解析请求
    int pos = str.find('\r', now_read_pos);
    if (pos < 0)
    {
        return PARSE_URI_AGAIN;
    }
    // 去掉请求行所占的空间，节省空间
    string request_line = str.substr(0, pos);
    if (str.size() > pos + 1)
        str = str.substr(pos + 1);
    else 
        str.clear();

    //从http请求中提取动作，只有可能是get（请求数据） 或者 post (上传数据)两种
    // Method
    pos = request_line.find("GET");
    if (pos < 0)
    {
        pos = request_line.find("POST");
        if (pos < 0)
        {
            return PARSE_URI_ERROR;
        }
        else
        {
            method = METHOD_POST;
        }
    }
    else
    {
        method = METHOD_GET;
    }
    //printf("method = %d\n", method);
    // filename
    pos = request_line.find("/", pos);
    if (pos < 0)
    {
        return PARSE_URI_ERROR;
    }
    else
    {
        int _pos = request_line.find(' ', pos);
        if (_pos < 0)
            return PARSE_URI_ERROR;
        else
        {
            if (_pos - pos > 1)
            {
                file_name = request_line.substr(pos + 1, _pos - pos - 1);
                int __pos = file_name.find('?');
                if (__pos >= 0)
                {
                    file_name = file_name.substr(0, __pos);
                }
            }
            else
                file_name = "/home/r/Mysoftware/neatWebserver/index.html";
        }
        pos = _pos;
    }
    //cout << "file_name: " << file_name << endl;
    // HTTP 版本号
    pos = request_line.find("/", pos);
    if (pos < 0)
    {
        return PARSE_URI_ERROR;
    }
    else
    {
        if (request_line.size() - pos <= 3)
        {
            return PARSE_URI_ERROR;
        }
        else
        {
            string ver = request_line.substr(pos + 1, 3);
            if (ver == "1.0")
                HTTPversion = HTTP_10;
            else if (ver == "1.1")
                HTTPversion = HTTP_11;
            else
                return PARSE_URI_ERROR;
        }
    }
    state = STATE_PARSE_HEADERS;
    return PARSE_URI_SUCCESS;
}

//解析完请求行以后parse_URI，开始解析请求头
//对请求头的解析，解析键值对headers
//如果Header成功解析，那么content中的内容就是空行和请求体
int requestData::parse_Headers()
{
    string &str = content;
    int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
    int now_read_line_begin = 0;
    bool notFinish = true;
    for (int i = 0; i < str.size() && notFinish; ++i)
    {
        switch(h_state)
        {
            case h_start:
            {
                if (str[i] == '\n' || str[i] == '\r')
                    break;
                h_state = h_key;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
            case h_key:
            {
                if (str[i] == ':')
                {
                    key_end = i;
                    if (key_end - key_start <= 0)
                        return PARSE_HEADER_ERROR;
                    h_state = h_colon;
                }
                else if (str[i] == '\n' || str[i] == '\r')
                    return PARSE_HEADER_ERROR;
                break;  
            }
            case h_colon:
            {
                if (str[i] == ' ')
                {
                    h_state = h_spaces_after_colon;
                }
                else
                    return PARSE_HEADER_ERROR;
                break;  
            }
            case h_spaces_after_colon:
            {
                h_state = h_value;
                value_start = i;
                break;  
            }
            case h_value:
            {
                if (str[i] == '\r')
                {
                    h_state = h_CR;
                    value_end = i;
                    if (value_end - value_start <= 0)
                        return PARSE_HEADER_ERROR;
                }
                else if (i - value_start > 255)
                    return PARSE_HEADER_ERROR;
                break;  
            }
            case h_CR:
            {
                if (str[i] == '\n')
                {
                    h_state = h_LF;
                    string key(str.begin() + key_start, str.begin() + key_end);
                    string value(str.begin() + value_start, str.begin() + value_end);
                    headers[key] = value;
                    now_read_line_begin = i;
                }
                else
                    return PARSE_HEADER_ERROR;
                break;  
            }
            case h_LF:
            {
                if (str[i] == '\r')
                {
                    h_state = h_end_CR;
                }
                else
                {
                    key_start = i;
                    h_state = h_key;
                }
                break;
            }
            case h_end_CR:
            {
                if (str[i] == '\n')
                {
                    h_state = h_end_LF;
                }
                else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_end_LF:
            {
                notFinish = false;
                key_start = i;
                now_read_line_begin = i;
                break;//整个头部已经解读完成，下面是空行和请求体了
            }
        }
    }
    if (h_state == h_end_LF)
    {
        str = str.substr(now_read_line_begin);
        return PARSE_HEADER_SUCCESS;
    }
    str = str.substr(now_read_line_begin);
    return PARSE_HEADER_AGAIN;
}

int requestData::analysisRequest()
{
    //把数据上传到服务器
    if (method == METHOD_POST)
    {
        //get content
        char header[MAX_BUFF];
        sprintf(header, "HTTP/1.1 %d %s\r\n", 200, "OK");
        //先在header中写入http版本信息
        if(headers.find("Connection") != headers.end() && headers["Connection"] == "keep-alive")
        {
            keep_alive = true;
            sprintf(header, "%sConnection: keep-alive\r\n", header);
            sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, EPOLL_WAIT_TIME);
        }//header中写入连接的状态

        //cout << "content=" << content << endl;
        // test char*
        char *send_content = "I have receiced this.";

        sprintf(header, "%sContent-Length: %zu\r\n", header, strlen(send_content));
        sprintf(header, "%s\r\n", header);
        //写入header信息

        size_t send_len = (size_t)writen(fd, header, strlen(header));//把header写入fd?

        if(send_len != strlen(header))
        {
            perror("Send header failed");
            return ANALYSIS_ERROR;
        }
        
        send_len = (size_t)writen(fd, send_content, strlen(send_content));
        //无关紧要的信息写入fd
        if(send_len != strlen(send_content))
        {
            perror("Send content failed");
            return ANALYSIS_ERROR;
        }
        cout << "content size =" << content.size() << endl;
        vector<char> data(content.begin(), content.end());//将请求体内容编码


{
    std::string str(data.begin(), data.end());
    size_t pos = 0;
    for(int sus=0;sus<=2;sus++)
    {
        pos = str.find('\r', pos);
        str = str.substr(pos);
    }


    // 将修改后的 string 转换为 vector<char>
    std::vector<char> result_vector(str.begin(), str.end());
    data = result_vector;
}

        for(char iii:data)std::cout<<iii;




// {
//     ifstream fff("/home/r/1230.png", ios::binary | ios::ate);
//     if (!fff.is_open()) {
//         cerr << "Failed to open file" << endl;
//         return -1;
//     }
//     // 获取文件大小
//     streamsize sizefff = fff.tellg();
//     fff.seekg(0, ios::beg);
//     // 存储文件数据的 vector
//     vector<char> bufferff(sizefff);
//     if (!fff.read(bufferff.data(), sizefff)) {
//         cerr << "Failed to read file" << endl;
//         return -1;
//     }
//     fff.close();
//     // 使用 imdecode 解码图像
//     Mat image = imdecode(bufferff, IMREAD_COLOR);
//     if (image.empty()) {
//         cerr << "imdecode failed" << endl;
//         // 可以添加更多的错误检查和处理逻辑，如检查 data 中的数据
//     } 
//         for(char iii:bufferff)std::cout<<iii;
// }

    Mat test = imdecode(data, cv::IMREAD_ANYDEPTH | cv::IMREAD_ANYCOLOR);
    if (test.empty()) {
        cerr << "imdecode failed, the input data may be invalid or incomplete." << endl;
        // 可以添加更多的错误检查和处理逻辑，如检查 data 中的数据
    } else {
        imwrite("/home/r/receive.bmp", test);
    }

        return ANALYSIS_SUCCESS;
    }
    else if (method == METHOD_GET)
    {
        char header[MAX_BUFF];
        sprintf(header, "HTTP/1.1 %d %s\r\n", 200, "OK");
        if(headers.find("Connection") != headers.end() && headers["Connection"] == "keep-alive")
        {
            keep_alive = true;
            sprintf(header, "%sConnection: keep-alive\r\n", header);
            sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, EPOLL_WAIT_TIME);
        }
        //先在header中写入http版本信息
        //header中写入连接的状态        
        int dot_pos = file_name.find('.');
        const char* filetype;
        if (dot_pos < 0)
            filetype = MimeType::getMime("default").c_str();
        else
            filetype = MimeType::getMime(file_name.substr(dot_pos)).c_str();
        //解析GET需要访问的文件类型是什么
        struct stat sbuf;
        std::cout<<"the file type is "<<filetype<<std::endl;
        std::cout<<"the file name is "<<file_name<<std::endl;

        if (stat(file_name.c_str(), &sbuf) < 0)//这个函数的作用是获取一个文件的状态信息
        {
            handleError(fd, 404, "Not Found!");
            return ANALYSIS_ERROR;
        }

        //文件类型写入header
        sprintf(header, "%sContent-type: %s\r\n", header, filetype);
        //文件大小写入header
        sprintf(header, "%sContent-Length: %ld\r\n", header, sbuf.st_size);
        sprintf(header, "%s\r\n", header);
        size_t send_len = (size_t)writen(fd, header, strlen(header));
        //将header写入fd
        if(send_len != strlen(header))
        {
            perror("Send header failed");
            return ANALYSIS_ERROR;
        }
        int src_fd = open(file_name.c_str(), O_RDONLY, 0);
        char *src_addr = static_cast<char*>(mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0));
        close(src_fd);
        //把这个文件file_name映射到一个内存空间中，并且返回内存的首地址。这样就可以像访问内存一样访问文件了
    
        // 发送文件并校验完整性
        send_len = writen(fd, src_addr, sbuf.st_size);
        //把文件内容发送到fd中
        if(send_len != sbuf.st_size)
        {
            perror("Send file failed");
            return ANALYSIS_ERROR;
        }
        munmap(src_addr, sbuf.st_size);//释放之前映射的一片空间
        return ANALYSIS_SUCCESS;
    }
    else
        return ANALYSIS_ERROR;
}

void requestData::handleError(int fd, int err_num, string short_msg)
{
    short_msg = " " + short_msg;
    char send_buff[MAX_BUFF];
    string body_buff, header_buff;
    body_buff += "<html><title>TKeed Error</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += to_string(err_num) + short_msg;
    body_buff += "<hr><em> LinYa's Web Server</em>\n</body></html>";

    header_buff += "HTTP/1.1 " + to_string(err_num) + short_msg + "\r\n";
    header_buff += "Content-type: text/html\r\n";
    header_buff += "Connection: close\r\n";
    header_buff += "Content-Length: " + to_string(body_buff.size()) + "\r\n";
    header_buff += "\r\n";
    sprintf(send_buff, "%s", header_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
    sprintf(send_buff, "%s", body_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
}

mytimer::mytimer(requestData *_request_data, int timeout): deleted(false), request_data(_request_data)
{
    //cout << "mytimer()" << endl;
    struct timeval now;
    gettimeofday(&now, NULL);
    // 以毫秒计
    expired_time = ((now.tv_sec * 1000) + (now.tv_usec / 1000)) + timeout;
}

mytimer::~mytimer()
{
    cout << "~mytimer()" << endl;
    if (request_data != NULL)
    {
        cout << "request_data=" << request_data << endl;
        delete request_data;
        request_data = NULL;
    }
}

bool mytimer::isvalid()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    size_t temp = ((now.tv_sec * 1000) + (now.tv_usec / 1000));
    if (temp < expired_time)
    {
        return true;
    }
    else
    {
        this->setDeleted();
        return false;
    }
}

void mytimer::clearReq()
{
    request_data = NULL;
    this->setDeleted();
}

void mytimer::setDeleted()
{
    deleted = true;
}

bool mytimer::isDeleted() const
{
    return deleted;
}

size_t mytimer::getExpTime() const
{
    return expired_time;
}

bool timerCmp::operator()(const mytimer *a, const mytimer *b) const
{
    return a->getExpTime() > b->getExpTime();
}