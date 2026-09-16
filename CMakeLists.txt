cmake_minimum_required(VERSION 3.16)

project(UniDB)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# --------------------------------
# GoogleTest
# --------------------------------

find_package(GTest REQUIRED)


# --------------------------------
# UniDB application
# --------------------------------

add_executable(unidb

    main.cpp

    # Container
    Container/Container.cpp

    # DiskManager
    DiskManager/adapter/FileDiskManagerAdapter.cpp

    DiskManager/actions/ReadPageAction.cpp
    DiskManager/actions/WritePageAction.cpp

    #BufferPoolManager
    BufferPoolManager/adapter/ReadPageAdapter.cpp
    BufferPoolManager/adapter/WritePageAdapter.cpp
    

    # TupleService
    TupleService/adapter/DiskManagerPageAdapter.cpp

    TupleService/actions/CreateTupleAction.cpp
    TupleService/actions/GetTupleAction.cpp
    TupleService/actions/DeleteTupleAction.cpp
    TupleService/actions/CreatePageAction.cpp
)


# --------------------------------
# Tests
# --------------------------------

add_executable(tuple_tests
    TupleService/test/TupleActionsTest.cpp
    # Container
    Container/Container.cpp

    # DiskManager
    DiskManager/adapter/FileDiskManagerAdapter.cpp
    DiskManager/actions/ReadPageAction.cpp
    DiskManager/actions/WritePageAction.cpp

    #BufferPoolManager
    BufferPoolManager/adapter/ReadPageAdapter.cpp
    BufferPoolManager/adapter/WritePageAdapter.cpp

    # TupleService
    TupleService/adapter/DiskManagerPageAdapter.cpp
    TupleService/actions/CreateTupleAction.cpp
    TupleService/actions/GetTupleAction.cpp
    TupleService/actions/DeleteTupleAction.cpp
    TupleService/actions/CreatePageAction.cpp
)


target_link_libraries(tuple_tests

    GTest::gtest
    GTest::gtest_main
)


enable_testing()

add_test(
    NAME TupleActionsTest
    COMMAND tuple_tests
)