cmake_minimum_required(VERSION 3.23)
project(autohdr_force C)

set(CMAKE_C_STANDARD 11)

add_executable(autohdr_force main.c)

target_link_libraries(autohdr_force "$<$<CONFIG:Release>:-s>")
