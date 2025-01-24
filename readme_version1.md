# 客户端操作
1. 先和服务器建立TCP链接
2. 将请求数据以http格式发给服务器
3. 接收服务器响应

# 服务器操作
1. 同时管理多个客户端链接
2. 同时处理多个客户端http请求
3. 将响应以http格式发送给客户

# HTTP请求示例
[https://blog.csdn.net/weixin_44322824/article/details/143925047?sharetype=blogdetail&sharerId=143925047&sharerefer=PC&sharesource=weixin_44322824&spm=1011.2480.3001.8118](https://blog.csdn.net/weixin_44322824/article/details/143925047?sharetype=blogdetail&sharerId=143925047&sharerefer=PC&sharesource=weixin_44322824&spm=1011.2480.3001.8118)

# 第一个版本Webserver的设计
1. **一个主线程**：
    - 处理客户端的连接请求
    - 将客户端的业务需求分配给各个线程
    - 维系连接，处理过期的请求和连接
2. **N个线程**：
    - 负责处理连接后客户端的业务需求

为了负载上的均衡，主线程是将业务依次分配给各个线程。

# 链接相关的核心过程
1. `epoll_init()`——epoll_fd文件描述符
2. socket创建listenfd，`bind()`——确定一个端口处理所有链接请求，这个这个直接确定了网页的地址，比如127.0.0.1:8888，这个8888就是绑定的端口
3. 开始循环：
    - `epoll_wait()`，看epoll_fd管理的端口有哪些发生了事件
    - 处理事件：
        - 是listenfd：有新的连接到来
            - `accept()`分配fd_n给新的连接
            - `epoll_add`把fd_n交给epoll_fd管理
        - 不是listenfd：已经创建的链接发生了可读写事件
    - 处理过期链接

# 问题与回答
**问题**：从客户端与服务器无链接状态开始，客户端发送一次请求，到服务器给出响应，epoll_wait会被触发几次呢？

**回答**：两次。
1. 客户端输入网址，敲下回车键后，首先会开始TCP三次握手。握手结束以后，listenfd会触发可读事件，`epoll_wait()`退出阻塞状态，我们通过`accept`为链接分配新的文件描述符fd_n，并且交给epoll_fd管理。
2. 客户继续发送htttp请求，等到循环再次来到`epoll_wait`时，我们发现fd_n出现了可读事件，我们就开始解析http，处理业务逻辑，返回给客户端。