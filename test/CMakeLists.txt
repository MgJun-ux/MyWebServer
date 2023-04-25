cmake_minimum_required(VERSION 3.13)    
project(mytest)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")




add_executable(mytest test.cpp)
# Link headers from other_project
target_include_directories(mytest 
    PUBLIC /home/mgjun/桌面/MyWebServer/include)

# Link source files from other_project
target_sources(mytest
    PRIVATE
        /home/mgjun/桌面/MyWebServer/src/log.cpp
        /home/mgjun/桌面/MyWebServer/include/log.h
        /home/mgjun/桌面/MyWebServer/src/buffer.cpp
        /home/mgjun/桌面/MyWebServer/include/buffer.h
)