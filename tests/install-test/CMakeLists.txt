cmake_minimum_required(VERSION 3.10)

project(KassertInstallTest)

find_package(kassert REQUIRED CONFIG)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE kassert::kassert)
