cmake_minimum_required(VERSION 3.0.0)
project(immer-example VERSION 0.1.0)

find_package(Immer CONFIG REQUIRED)

add_executable(immer-example main.cpp)
target_link_libraries(immer-example PRIVATE immer)