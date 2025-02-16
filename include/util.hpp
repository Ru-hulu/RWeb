#include<sys/socket.h>
#include<fcntl.h>
#include<signal.h>
#include<memory.h>
#include<iostream>
#include<unistd.h>
#include<error.h>

#ifndef UTIL_HPP
#define UTIL_HPP
const uint32_t TIMEOUT = 1;
int socketNonBlock(int fd);
bool handle_sigpipe();
ssize_t read_n(int fd_,void* bf,size_t n);
ssize_t write_n(int fd_,void* bf,size_t n);
class noncopyable
{
    protected://集成可以调用
        noncopyable() = default;
        ~noncopyable() = default;
        noncopyable(const noncopyable &a)=delete;//拷贝构造函数禁用
        noncopyable(noncopyable&& a)=delete;//移动构造函数禁用
        noncopyable& operator=(const noncopyable& a)=delete;//拷贝运算符禁用
        noncopyable& operator=(noncopyable&& a)=delete;//移动运算符禁用
};
#endif