cmake_minimum_required(VERSION 3.10)
project(myserver)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -O3")

# 查找 OpenCV 库
find_package(OpenCV 4.2.0 REQUIRED)

# 查找线程库
find_package(Threads REQUIRED)

# 包含目录


include_directories(
    ${OpenCV_INCLUDE_DIRS}    
    include
)
link_directories(
    ${OpenCV_LIBRARY_DIRS}
)
# 源文件
file(GLOB SOURCE_FILES "src/*.cpp")

# 生成可执行文件
add_executable(myserver ${SOURCE_FILES})
# 链接库
target_link_libraries(myserver ${OpenCV_LIBRARIES} Threads::Threads)