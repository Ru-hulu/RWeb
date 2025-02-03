#include"requestData.hpp"
#include"MemoryPool.hpp"
#include<unistd.h>
#include<sys/stat.h>
#include <sys/mman.h>
#include<fstream>
std::unordered_map<std::string,std::string> mime = {
            {".html"   , "text/html"},
            {".avi"    , "video/x-msvideo"},
            {".bmp"    , "image/bmp"},
            {".c"     , "text/plain"},
            {".doc"    , "application/msword"},
            {".gif"    , "image/gif"},
            {".gz"     , "application/x-gzip"},
            {".htm"    , "text/html"},
            {".ico"    , "application/x-ico"},
            {".jpg"    , "image/jpeg"},
            {".png"    , "image/png"},
            {".txt"    , "text/plain"},
            {".mp3"    , "audio/mp3"},
            {"default" , "text/html"}
}; 
myTimer::myTimer(requestData* rqtp_,size_t timeoutms):rqtp(rqtp_)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    expired_time = tv.tv_sec*1000 + tv.tv_usec/1000 + timeoutms;
    return;
}
int myTimer::get_fd()
{
    if(rqtp==nullptr)
    return -1;
    return rqtp->getFd();
}
myTimer::~myTimer()
{
    //析沟函数中需要释放requestData;
    if(rqtp!=nullptr)
    {
        // if(rqtp->in_q)
        // while(1)
        // std::cout<<"her1";
        std::cout<<"FD "<<rqtp->getFd()<<" is freed"<<std::endl;
        MemoryManager::deleteElement<requestData>(rqtp);
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
    h_state = h_start;
}
requestData::~requestData()
{
    int ev_id = EPOLLIN|EPOLLET|EPOLLONESHOT;
    epoll_del(epoll_fd,fd,ev_id);//删除epoll_fd 与 fd的关联
    close(fd);//关闭文件描述符
    // std::cout<<"now free fd is "<<fd<<std::endl;
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
    HTTPversion = -1;
    state_handle = -1;
    h_state = h_start;
}
int requestData::parse_URI()
{   
    std::string &str = content;
    // 读到完整的请求行再开始解析请求
    int pos = str.find('\r', 0);
    std::cout<<"content size is "<<content.size()<<std::endl;
    if (pos < 0)
    {
        return PARSE_URI_AGAIN;
    }
    // 去掉请求行所占的空间，节省空间
    std::string request_line = str.substr(0, pos);
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
                file_name = "index.html";
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
            std::string ver = request_line.substr(pos + 1, 3);
            if (ver == "1.0")
                HTTPversion = HTTP_10;
            else if (ver == "1.1")
                HTTPversion = HTTP_11;
            else
                return PARSE_URI_ERROR;
        }
    }
    state_handle = STATE_PARSE_HEADERS;
    return PARSE_URI_SUCCESS;
}

int requestData::parse_HEADER()
{   
    std::string &str = content;
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
                    std::string key(str.begin() + key_start, str.begin() + key_end);
                    std::string value(str.begin() + value_start, str.begin() + value_end);
                    header_map[key] = value;
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
std::string requestData::getMime(std::string t)
{
    if(mime.find(t)==mime.end())
    {
        return mime["default"];
    }
    return mime[t];
}

//状态行、响应头、响应体
void requestData::handleError(int fd, int err_num, std::string short_msg)
{
    short_msg = " " + short_msg;
    char send_buff[MAX_BUFF];
    std::string body_buff, header_buff;
    body_buff += "<html><title>TKeed Error</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += std::to_string(err_num) + short_msg;
    body_buff += "<hr><em> LinYa's Web Server</em>\n</body></html>";

    header_buff += "HTTP/1.1 " + std::to_string(err_num) + short_msg + "\r\n";//状态行
    header_buff += "Content-type: text/html\r\n";
    header_buff += "Connection: close\r\n";
    header_buff += "Content-Length: " + std::to_string(body_buff.size()) + "\r\n";
    //响应头
    header_buff += "\r\n";//空行
    sprintf(send_buff, "%s", header_buff.c_str());
    write_n(fd, send_buff, strlen(send_buff));
    sprintf(send_buff, "%s", body_buff.c_str());
    write_n(fd, send_buff, strlen(send_buff));
}

int requestData::analysisRequest()
{
    //std::cout<<"method is "<<method<<std::endl;
    //把数据上传到服务器
    if (method == METHOD_POST)
    {
        //get content
        char header[MAX_BUFF];
        sprintf(header, "HTTP/1.1 %d %s\r\n", 200, "OK");
        //先在header中写入http版本信息
        if(header_map.find("Connection") != header_map.end() && header_map["Connection"] == "keep-alive")
        {
            keepalive = true;
            sprintf(header, "%sConnection: keep-alive\r\n", header);
            sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, 500);
        }//header中写入连接的状态

        // cout << "content=" << content << endl;
        // test char*
        char *send_content = "I have receiced this.";

        sprintf(header, "%sContent-Length: %zu\r\n", header, strlen(send_content));
        sprintf(header, "%s\r\n", header);
        //写入header信息

        size_t send_len = (size_t)write_n(fd, header, strlen(header));//把header写入fd?

        if(send_len != strlen(header))
        {
            perror("Send header failed");
            return ANALYSIS_ERROR;
        }
        
        send_len = (size_t)write_n(fd, send_content, strlen(send_content));
        //无关紧要的信息写入fd
        if(send_len != strlen(send_content))
        {
            perror("Send content failed");
            return ANALYSIS_ERROR;
        }
        std::ofstream t_f;
        std::unique_lock<std::mutex> nqm(f_n_mtx);
        f_p_n++;
        nqm.unlock();
        t_f.open("/home/r/Mysoftware/RWeb/postfile/"+std::to_string(f_p_n)+".txt");
        if(t_f.is_open())
        {
            t_f<<content;
            t_f.close();
            //std::cout<<"write success."<<std::endl;
        }
        return ANALYSIS_SUCCESS;
    }
    else if (method == METHOD_GET)
    {
        char header[MAX_BUFF];
        sprintf(header, "HTTP/1.1 %d %s\r\n", 200, "OK");
        if(header_map.find("Connection") != header_map.end() && header_map["Connection"] == "keep-alive")
        {
            keepalive = true;
            sprintf(header, "%sConnection: keep-alive\r\n", header);
            sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, 500);
        }
        //先在header中写入http版本信息
        //header中写入连接的状态        
        int dot_pos = file_name.find('.');
        const char* filetype;
        if (dot_pos < 0) 
            filetype = getMime("default").c_str();
        else
            filetype = getMime(file_name.substr(dot_pos)).c_str();
        //解析GET需要访问的文件类型是什么
        file_name = "/home/r/Mysoftware/RWeb/index.html";
        struct stat sbuf;
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
        size_t send_len = (size_t)write_n(fd, header, strlen(header));
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
        send_len = write_n(fd, src_addr, sbuf.st_size);
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
void requestData::handlerequest()
{
    std::cout<<"this is "<<this<<std::endl;
    bool isError = false;
    char buf[MAX_BUFF];
    while(true)
    {
            int stat_f  = fcntl(fd,F_GETFL,0);
            size_t r = read_n(fd,buf,MAX_BUFF);
            if(r<0)
            {
                isError = true;
                //std::cout<<"read_n failed"<<std::endl;
                break;
                //如果读的过程中出错了，那么直接删除这个连接
            }
            else if(errno==EAGAIN&&r==0)
            {
                if(readagaintime>=AGAIN_MAX_TIMES)
                {
                    //当前的连接出错，直接作废。    
                    //std::cout<<"read again time out"<<std::endl;
                    isError = true;
                }
                else
                {
                    //等待下一次触发,content不清空
                    readagaintime++;
                    //std::cout<<"read_n again"<<std::endl;
                }
                break;
            }
            else if(r==0&&errno==0)
            {
                isError = true;
                break;//对端关闭链接。
                
            }
            // std::cout<<"r is "<<r<<"errno is "<<errno<<std::endl;
            //如果读到了数据：
            std::string now_read(buf,buf+r);
            content += now_read;
            if(state_handle==STATE_PARSE_URI)
            {
                int st = parse_URI();
                // std::cout<<"parse_URI is"<<st<<std::endl;
                if(st==PARSE_URI_AGAIN)
                {//提示当前的数据还不够完整，需要再读数据
                    break;
                }
                else if(st==PARSE_URI_ERROR)
                {
                    isError = true;
                    //std::cout<<"PARSE_URI_ERROR"<<std::endl;
                    break;
                }
                else if(st == PARSE_URI_SUCCESS)
                state_handle = STATE_PARSE_HEADERS;
            }//URI已经解析成功
            if(state_handle==STATE_PARSE_HEADERS)
            {
                int st = parse_HEADER();
                // std::cout<<"Now method is "<< method <<std::endl;
                if(st == PARSE_HEADER_AGAIN)
                {//提示当前的数据还不够完整，需要再读数据
                    break;
                }
                else if(st == PARSE_HEADER_ERROR)
                {
                    isError = true;
                    //std::cout<<"PARSE_HEADER_ERROR"<<std::endl;
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
                    //std::cout<<"STATE_RECV_BODY"<<std::endl;                    
                    break;
                }
                int con_size = std::stoi(header_map["Content-Length"]);
                if(content.size()<con_size)
                continue;//这个逻辑不是很封闭。需要改进。
                state_handle = STATE_ANALYSIS;
            }
            if(state_handle == STATE_ANALYSIS)
            {
                //std::cout<<"handlerequest: Start analysisRequest"<<std::endl;
                int a_ret = analysisRequest();//这里说明需要处理的数据已经全部在content里面了，
                if(a_ret == ANALYSIS_SUCCESS)
                {
                    std::cout<<"handlerequest: Success analysisRequest"<<std::endl;
                    state_handle=STATE_FINISH;
                }
                else
                {
                    std::cout<<"handlerequest: Fail analysisRequest"<<std::endl;
                    isError = true;
                }
                break;                    
            }

    }
    //每次都是parse—url-again 然后break到这里重新设定epoll
        if(isError)
        {
            //std::cout<<"handlerequest Error"<<std::endl;
            MemoryManager::deleteElement<requestData>(this);
            return;
        }
        if(state_handle==STATE_FINISH)
        {
            //std::cout<<"handlerequest Finish"<<std::endl;
            if(keepalive)
            {
                reset_data();
                //std::cout<<"now is kept alive"<<std::endl;
            }
            else
            {
                //std::cout<<"now is not kept alive"<<std::endl;
                MemoryManager::deleteElement<requestData>(this);
                return;
            }
        }
        myTimer* tm = MemoryManager::newElement<myTimer>(this,500);
        // myTimer* tm = new myTimer(this,500);

        mtimer = tm;
        std::unique_lock<std::mutex>llkk(mutex_timer_q);
        Prio_timer_queue.push(tm);
        llkk.unlock();
        //上述的功能应该已经处理好了，接下来应该是连接保留的工作

        int ev_id = EPOLLIN|EPOLLONESHOT|EPOLLET;
        int ret = epoll_mod(epoll_fd,this,fd,ev_id);
        if(ret<0)
        {
            std::cout<<"this is deleted "<<std::endl;
            MemoryManager::deleteElement<requestData>(this);
            return;
        }
}