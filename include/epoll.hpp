#include<sys/epoll.h>
#include<iostream>
#ifndef EPOLL_HPP
#define EPOLL_HPP
#define MAXEVENT 5000
const int LISTENQ = 1024;
extern struct epoll_event* events_get;
int epoll_init();
int epoll_add(int epoll_fd_,void* request_data_,int fd_,uint32_t events_id);
int epoll_mod(int epoll_fd_,void* request_data_,int fd_,uint32_t events_id);
int epoll_del(int epoll_fd_,int fd_,uint32_t events_id);
int my_epoll_wait(int epoll_fd_,struct epoll_event* events, uint32_t time_out,uint32_t maxevents);
#endif