#include"Logger.hpp"
#include<string>
#include<cstring>
#include<time.h>
typedef Log::Logger::LogStream LogStream;

LogStream& Log::Logger::stream()
{
    return stream_;
}
LogStream& LogStream::operator<<(short i)
{
    std::string thiss = std::to_string(i);
    return *this<<thiss;
};
LogStream& LogStream::operator<<(unsigned short i)
{
    std::string thiss = std::to_string(i);
    return *this<<thiss;
}
LogStream& LogStream::operator<<(int i)
{
    std::string thiss = std::to_string(i);
    return *this<<thiss;
};
LogStream& LogStream::operator<<(unsigned int i)
{
    std::string thiss = std::to_string(i);
    return *this<<thiss;
};
LogStream& LogStream::operator<<(long i)
{
    std::string thiss = std::to_string(i);
    return *this<<thiss;
};
LogStream& LogStream::operator<<(unsigned long i)
{
    std::string thiss = std::to_string(i);
    return *this<<thiss;
};
LogStream& LogStream::operator<<(long long i)
{
    std::string thiss = std::to_string(i);
    return *this<<thiss;
};
LogStream& LogStream::operator<<(unsigned long long i)
{
    std::string thiss = std::to_string(i);
    return *this<<thiss;
};
LogStream& LogStream::operator<<(float i)
{
    std::string thiss = std::to_string(i);
    return *this<<thiss;
};
LogStream& LogStream::operator<<(double i)
{
    std::string thiss = std::to_string(i);
    return *this<<thiss;
};
LogStream& LogStream::operator<<(const void* p)
{
    uintptr_t ptr_int = (uintptr_t)(p);
    if(64>get_rest()) return *this;
    sprintf(cur_,"%x",ptr_int);
    cur_+=64;
    return *this;
};

LogStream& LogStream::operator<<(char c)
{
    if(get_rest()==0) return *this;
    *(cur_++) = c;
    return *this;
};

LogStream& LogStream::operator<<(const char* c)
{
    if(strlen(c)>get_rest())
    return *this;
    sprintf(cur_,"%s",c);
    cur_ += strlen(c);
};

LogStream& LogStream::operator<<(std::string thiss)
{
    if(get_rest()<thiss.size())
    return *this;
    for(char c:thiss)
    *(cur_++) = c;
    return *this;
};
inline char* LogStream::get_cur()
{
    return cur_;
}
inline int LogStream::get_rest()
{
    return (Log::BUFF_SIZE - (cur_-buff_f));
}
LogStream::~LogStream()
{
    CoutFunction(buff_f,cur_-buff_f);
    if(Log::get_level()==Log::FATAL)
    {
        FlushFunction();        
        abort();
    }
}
void Log::DefaultWrite(char* d,int l)
{
    ssize_t n = fwrite(d,1,l,stdout);
    (void) n;
}
void Log::DefaultFlush()
{
    fflush(stdout);
}
void Log::set_level(Log::LogLevel l)
{
    level_ = l;
}
Log::LogLevel Log::get_level()
{
    return level_;
}
Log::Logger::Logger(const char* file_n,int line_n,
Log::LogLevel l_,const char* func_n)
{
    set_level(l_);
    time_t tt;
    time(&tt);
    struct tm* st = localtime(&tt);
    char header[60];
    snprintf(header,60,
           "Current date and time: %04d-%02d-%02d %02d:%02d:%02d\n",
           st->tm_year + 1900, st->tm_mon + 1, st->tm_mday,
           st->tm_hour, st->tm_min, st->tm_sec);
    stream()<<"Time stamp "<<
    file_n<<"---"<<line_n<<"---"<<func_n<<"---";
}