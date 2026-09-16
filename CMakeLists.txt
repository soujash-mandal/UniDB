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

    # FreePageManager
    FreePageManager/domain/FreePageMetadata.cpp

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

add_executable(tests
    TupleService/test/TupleActionsTest.cpp
    # Container
    Container/Container.cpp

    # DiskManager
    DiskManager/adapter/FileDiskManagerAdapter.cpp
    DiskManager/actions/ReadPageAction.cpp
    DiskManager/actions/WritePageAction.cpp

    # FreePageManager
    FreePageManager/domain/FreePageMetadata.cpp
    FreePageManager/test/FreePageMetadataTest.cpp

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


target_link_libraries(tests

    GTest::gtest
    GTest::gtest_main
)


enable_testing()
include(GoogleTest)

gtest_discover_tests(tests)
