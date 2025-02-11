#include<iostream>
#include<string>
#ifndef LOGGER_HPP
#define LOGGER_HPP
namespace Log
{
    enum LogLevel
    {
        INFO,
        WARN,
        ERR,
        FATAL
    };
    void DefaultWrite(char*,int);
    void DefaultFlush();
    void SetCout(void (*cout_)(char*,int));
    void set_level(LogLevel l);
    LogLevel get_level();
    const int BUFF_SIZE = 1024;
    extern LogLevel level_;//当前日志输出的等级
    extern void(*CoutFunction)(char*,int);
    extern void(*FlushFunction)();
    class  Logger
    {
        public:
            Logger(const char*,int,LogLevel);
            class LogStream
            {
                public:
                    LogStream():cur_(buff_f){};
                    ~LogStream();//析构的时候需要写入到AsyncBuffer中
                    LogStream& operator<<(short);
                    LogStream& operator<<(unsigned short);
                    LogStream& operator<<(int);
                    LogStream& operator<<(unsigned int);
                    LogStream& operator<<(long);
                    LogStream& operator<<(unsigned long);
                    LogStream& operator<<(long long);
                    LogStream& operator<<(unsigned long long);
                    LogStream& operator<<(const void*);                
                    LogStream& operator<<(float);                
                    LogStream& operator<<(double);                
                    LogStream& operator<<(const char*);                
                    LogStream& operator<<(char);                
                    LogStream& operator<<(std::string);   
                    char* get_cur();//当前缓存区的指针     
                    int get_rest();           
                private:
                    char buff_f[BUFF_SIZE];//缓存区
                    char* cur_;//当前允许填充的位置                    
            };
            LogStream stream_;
            LogStream& stream();
    };
};
#define LOG_INFO if(Log::get_level()<=Log::INFO) \
Log::Logger(__FILE__,__LINE__,Log::INFO).stream()
#define LOG_WARN if(Log::get_level()<=Log::WARN) \
Log::Logger(__FILE__,__LINE__,Log::WARN).stream()
#define LOG_ERR if(Log::get_level()<=Log::ERR) \
Log::Logger(__FILE__,__LINE__,Log::ERR).stream()
#define LOG_FATAL if(Log::get_level()<=Log::FATAL) \
Log::Logger(__FILE__,__LINE__,Log::FATAL).stream()
#endif