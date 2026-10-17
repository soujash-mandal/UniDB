cmake_minimum_required(VERSION 3.16)

project(UniDB)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(GTest REQUIRED)

set(UNIDB_SOURCES
    container/container.cpp

    DiskManager/adapter/FileDiskManagerAdapter.cpp
    DiskManager/actions/ReadDiskPageAction.cpp
    DiskManager/actions/WriteDiskPageAction.cpp

    FreePageManager/adapter/ReadFreePageMetadataAdapter.cpp
    FreePageManager/adapter/WriteFreePageMetadataAdapter.cpp
    FreePageManager/domain/MetadataPage.cpp
    FreePageManager/actions/AllocatePageAction.cpp
    FreePageManager/actions/CreateFpmPageAction.cpp

    EvictionPolicy/EvictionPolicyFactory.cpp
    EvictionPolicy/FIFOEvictionPolicy.cpp

    BufferPoolManager/adapter/ReadPageAdapter.cpp
    BufferPoolManager/adapter/WritePageAdapter.cpp
    BufferPoolManager/adapter/AllocatePageAdapter.cpp
    BufferPoolManager/actions/FetchPageAction.cpp
    BufferPoolManager/actions/WritePageAction.cpp
    BufferPoolManager/actions/FlushPageAction.cpp
    BufferPoolManager/actions/NewPageAction.cpp
    BufferPoolManager/actions/UnpinPageAction.cpp

    RdbMetadataService/domain/RdbMetadataPage.cpp
    RdbMetadataService/adapter/BufferPoolFetchPageAdapter.cpp
    RdbMetadataService/adapter/BufferPoolNewPageAdapter.cpp
    RdbMetadataService/adapter/BufferPoolUnpinPageAdapter.cpp
    RdbMetadataService/adapter/BufferPoolWritePageAdapter.cpp
    RdbMetadataService/actions/CreateRdbMetadataPageAction.cpp
    RdbMetadataService/actions/UpdateRdbMetadataPageAction.cpp
    RdbMetadataService/actions/GetCatalogRootPageIdAction.cpp
    RdbMetadataService/actions/AllocateNextTableIdAction.cpp

    CatalogService/adapter/BufferPoolFetchPageAdapter.cpp
    CatalogService/adapter/BufferPoolNewPageAdapter.cpp
    CatalogService/adapter/BufferPoolUnpinPageAdapter.cpp
    CatalogService/adapter/BufferPoolWritePageAdapter.cpp
    CatalogService/domain/CatalogPage.cpp
    CatalogService/actions/CreateRootCatalogPageAction.cpp
    CatalogService/actions/CreateTableAction.cpp
    CatalogService/actions/GetTableAction.cpp
    CatalogService/actions/DropTableAction.cpp

    TablePageDirectory/domain/TablePageDirectoryPage.cpp

    TupleService/domain/TuplePage.cpp
    TupleService/adapter/BufferPoolFetchPageAdapter.cpp
    TupleService/adapter/BufferPoolNewPageAdapter.cpp
    TupleService/adapter/BufferPoolUnpinPageAdapter.cpp
    TupleService/adapter/BufferPoolWritePageAdapter.cpp
    TupleService/actions/CreateTuplePageAction.cpp
    TupleService/actions/CreateTupleAction.cpp
    TupleService/actions/GetTupleAction.cpp
    TupleService/actions/DeleteTupleAction.cpp

    RdbService/actions/InitializeRdbAction.cpp
    RdbService/adapter/CreateRdbMetadataPageAdapter.cpp
    RdbService/adapter/CreateRootCatalogPageAdapter.cpp
    RdbService/adapter/UpdateRdbMetadataPageAdapter.cpp

    SQLParser/SqlLexer.cpp
    SQLParser/SqlParserImpl.cpp
)

add_executable(unidb
    main.cpp
    DatabaseStartup.cpp
    ${UNIDB_SOURCES}
)

add_executable(tests
    DatabaseStartup.cpp
    ${UNIDB_SOURCES}

    DiskManager/test/DiskManagerE2ETest.cpp
    FreePageManager/test/TestFreePageMetadataPage.cpp
    EvictionPolicy/test/FIFOEvictionPolicyTest.cpp
    BufferPoolManager/test/FetchPageActionTest.cpp
    BufferPoolManager/test/UnpinPageActionTest.cpp
    BufferPoolManager/test/NewPageActionTest.cpp
    BufferPoolManager/test/FlushPageActionTest.cpp
    BufferPoolManager/test/FoundationIntegrationTest.cpp
    CatalogService/test/CatalogPageTest.cpp
    TablePageDirectory/test/TablePageDirectoryPageTest.cpp
    TupleService/test/TupleActionsTest.cpp
    SQLParser/test/SqlParserTest.cpp
)

target_link_libraries(tests
    GTest::gtest
    GTest::gtest_main
)

enable_testing()
include(GoogleTest)
gtest_discover_tests(tests)
