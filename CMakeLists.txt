cmake_minimum_required(VERSION 3.16)

project(UniDB)

set(CMAKE_CXX_STANDARD 20)
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
    FreePageManager/adapter/ReadFreePageMetadataAdapter.cpp
    FreePageManager/adapter/WriteFreePageMetadataAdapter.cpp
    FreePageManager/domain/FreePageMetadata.cpp
    FreePageManager/actions/AllocatePageAction.cpp
    FreePageManager/actions/FreePageAction.cpp

    #BufferPoolManager
    BufferPoolManager/adapter/ReadPageAdapter.cpp
    BufferPoolManager/adapter/WritePageAdapter.cpp
    BufferPoolManager/adapter/AllocatePageAdapter.cpp
    BufferPoolManager/actions/FetchPageAction.cpp
    BufferPoolManager/actions/FlushAllPagesAction.cpp
    BufferPoolManager/actions/FlushPageAction.cpp
    BufferPoolManager/actions/NewPageAction.cpp
    BufferPoolManager/actions/UnpinPageAction.cpp

    

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
    
    # Container
    Container/Container.cpp

    # DiskManager
    DiskManager/adapter/FileDiskManagerAdapter.cpp
    DiskManager/actions/ReadPageAction.cpp
    DiskManager/actions/WritePageAction.cpp

    # FreePageManager
    FreePageManager/adapter/ReadFreePageMetadataAdapter.cpp
    FreePageManager/adapter/WriteFreePageMetadataAdapter.cpp
    FreePageManager/domain/FreePageMetadata.cpp
    FreePageManager/actions/AllocatePageAction.cpp
    FreePageManager/actions/FreePageAction.cpp
    FreePageManager/test/FreePageMetadataTest.cpp
    FreePageManager/test/AllocatePageActionTest.cpp
    
    #BufferPoolManager
    BufferPoolManager/adapter/ReadPageAdapter.cpp
    BufferPoolManager/adapter/WritePageAdapter.cpp
    BufferPoolManager/adapter/AllocatePageAdapter.cpp
    BufferPoolManager/actions/FetchPageAction.cpp
    BufferPoolManager/actions/FlushAllPagesAction.cpp
    BufferPoolManager/actions/FlushPageAction.cpp
    BufferPoolManager/actions/NewPageAction.cpp
    BufferPoolManager/actions/UnpinPageAction.cpp
    BufferPoolManager/test/FetchPageActionTest.cpp
    BufferPoolManager/test/UnpinPageActionTest.cpp
    BufferPoolManager/test/FlushPageActionTest.cpp
    BufferPoolManager/test/NewPageActionTest.cpp

    # TupleService
    TupleService/adapter/DiskManagerPageAdapter.cpp
    TupleService/actions/CreateTupleAction.cpp
    TupleService/actions/GetTupleAction.cpp
    TupleService/actions/DeleteTupleAction.cpp
    TupleService/actions/CreatePageAction.cpp
    TupleService/test/TupleActionsTest.cpp
)


target_link_libraries(tests
    GTest::gtest
    GTest::gtest_main
)


enable_testing()
include(GoogleTest)

gtest_discover_tests(tests)
