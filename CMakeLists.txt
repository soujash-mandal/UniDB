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
    PageService/actions/CreatePageAction.cpp
)


# --------------------------------
# Tests
# --------------------------------

add_executable(tuple_tests
    PageService/test/TupleActionsTest.cpp
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
    PageService/actions/CreatePageAction.cpp
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