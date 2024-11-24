#include<sys/socket.h>
#include<fcntl.h>
#include<signal.h>
#include<memory.h>
#include<iostream>
#include<unistd.h>
#include<error.h>

#ifndef UTIL_HPP
#define UTIL_HPP
int socketNonBlock(int fd);
bool handle_sigpipe();
ssize_t read_n(int fd_,void* bf,size_t n);
ssize_t write_n(int fd_,void* bf,size_t n);
#endif