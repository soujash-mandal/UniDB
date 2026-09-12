cmake_minimum_required(VERSION 3.16)

project(UniDB)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(unidb
    main.cpp
    Container/Container.cpp
    DiskManagerService/adapter/FileDiskManagerAdapter.cpp
    DiskManagerService/actions/ReadPageAction.cpp
    DiskManagerService/actions/WritePageAction.cpp
)