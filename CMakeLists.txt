cmake_minimum_required(VERSION 3.10)
project(MemoryPool)
SET(CMAKE_BUILD_TYPE "Debug")
# add_executable(test src/testLFU.cpp src/MemoryPool.cpp src/LFUcache.cpp)//测试LFU和内存池
# add_executable(test src/testmem.cpp src/MemoryPool.cpp)//测试内存池
add_executable(test src/main.cpp)