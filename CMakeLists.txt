cmake_minimum_required(VERSION 3.10)
project(MemoryPool)
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
find_package(Threads REQUIRED)
include_directories(
    include)
# add_executable(test src/testLFU.cpp src/MemoryPool.cpp src/LFUcache.cpp)//测试LFU和内存池
# add_executable(test src/testmem.cpp src/MemoryPool.cpp)//测试内存池
# add_executable(test src/epoll.cpp)

add_executable(test src/main.cpp src/requestData.cpp src/util.cpp src/epoll.cpp src/ThreadPool.cpp)
target_link_libraries(test Threads::Threads)