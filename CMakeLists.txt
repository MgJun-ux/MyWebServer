cmake_minimum_required(VERSION 3.10)    
project(MyWebServer)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

include_directories(${PROJECT_SOURCE_DIR}/include)


aux_source_directory(./src SrcFiles)
link_directories(/usr/lib/x86_64-linux-gnu)

add_executable(${PROJECT_NAME} ${SrcFiles})


target_link_libraries(MyWebServer libmysqlclient.so)
