cmake_minimum_required(VERSION 3.16.3)
project(queueProject)

set(CMAKE_CXX_STANDARD 17)
link_libraries(jemalloc)
add_executable(queue main.cpp)

find_package(Threads REQUIRED)
target_link_libraries(queue PRIVATE Threads::Threads)