cmake_minimum_required(VERSION 3.16)

project(UniDB)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(unidb

    main.cpp

    # Container
    Container/Container.cpp

    # DiskManagerService
    DiskManagerService/adapter/FileDiskManagerAdapter.cpp

    DiskManagerService/actions/ReadPageAction.cpp
    DiskManagerService/actions/WritePageAction.cpp

    # PageService
    PageService/adapter/DiskManagerPageAdapter.cpp

    PageService/actions/CreateTupleAction.cpp
    PageService/actions/ReadTupleAction.cpp
    PageService/actions/UpdateTupleAction.cpp
    PageService/actions/DeleteTupleAction.cpp
)