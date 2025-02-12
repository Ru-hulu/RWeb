cmake_minimum_required(VERSION 3.10)
project(MemoryPool)
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
find_package(Threads REQUIRED)
include_directories(
    include)
set(
    BUILDCPP
    src/MemoryPool.cpp
    src/requestData.cpp
    src/ThreadPool.cpp
    src/epoll.cpp
    src/main.cpp
    src/util.cpp
    src/AsyncLogging.cpp
    src/Logger.cpp
    )

    # 内存池测试
    # set(
    # BUILDCPP
    # src/MemoryPool.cpp
    # src/testmem.cpp
    # )    
    # 内存池测试

    # LOG文件测试
    # set(
    #     BUILDCPP
    #     src/AsyncLogging.cpp
    #     src/Logger.cpp
    #     src/Log_test.cpp
    #     )    
    # LOG文件测试

add_executable(test ${BUILDCPP})
target_link_libraries(test Threads::Threads)