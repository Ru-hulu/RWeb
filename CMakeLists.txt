cmake_minimum_required(VERSION 3.10)
project(MemoryPool)
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
find_package(Threads REQUIRED)
include_directories(
    include)
set(
    BUILDCPP
    # src/MemoryPool.cpp
    # src/requestData.cpp
    # src/ThreadPool.cpp
    # src/epoll.cpp
    # src/main.cpp
    # src/util.cpp
    src/AsyncLogging.cpp
    src/Logger.cpp
    src/Log_test.cpp
    )
# add_executable(test src/easy.cpp)
add_executable(test ${BUILDCPP})
target_link_libraries(test Threads::Threads)