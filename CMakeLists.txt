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
    CatalogService/actions/GetTableByNameAction.cpp
    CatalogService/actions/UpdateTableAction.cpp
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
    BufferPoolManager/test/UnpinPageAction/LastUnpinMakesPageEvictableTest.cpp
    BufferPoolManager/test/UnpinPageAction/PageRemainsNonEvictableWhileAnotherPinExistsTest.cpp
    BufferPoolManager/test/UnpinPageAction/UnknownPageThrowsWithoutChangingResidentPageTest.cpp
    BufferPoolManager/test/UnpinPageAction/RejectsPinCountUnderflowTest.cpp
    BufferPoolManager/test/UnpinPageAction/PolicyFailurePreservesFinalPinForRetryTest.cpp
    BufferPoolManager/test/UnpinPageAction/ReplacementDoesNotInheritVictimPinTest.cpp
    BufferPoolManager/test/UnpinPageAction/ReplacementWaitsForFinalUnpinToFinishTest.cpp
    BufferPoolManager/test/UnpinPageAction/PolicyFailureAfterReplacementDoesNotPinReplacementTest.cpp
    BufferPoolManager/test/NewPageActionTest.cpp
    BufferPoolManager/test/FlushPageActionTest.cpp
    BufferPoolManager/test/WritePageActionTest.cpp
    BufferPoolManager/test/FoundationIntegrationTest.cpp
    CatalogService/test/CatalogPage/RemovingTableReclaimsSpaceAndPreservesOtherTablesTest.cpp
    CatalogService/test/CatalogPage/RemovingFirstEntryPreservesRemainingTablesTest.cpp
    CatalogService/test/CatalogPage/RemovingMiddleEntryPreservesRemainingTablesTest.cpp
    CatalogService/test/CatalogPage/RemovingLastEntryPreservesRemainingTablesTest.cpp
    CatalogService/test/CatalogPage/RemovingOnlyEntryLeavesPageEmptyAndReusableTest.cpp
    CatalogService/test/CatalogPage/RemovingMissingTableThrowsWithoutChangingPageTest.cpp
    CatalogService/test/CatalogPage/LooksUpAndUpdatesTableMetadataByNameTest.cpp
    CatalogService/test/CatalogPage/RepeatedGrowingUpdatesReuseCompactedPayloadSpaceTest.cpp
    CatalogService/test/CatalogPage/ReadsLegacyTablePayloadWithoutFreeSpaceMapPageIdTest.cpp
    CatalogService/test/CatalogPage/CatalogPageTestSupport.h
    TupleService/test/TupleActionsTest.cpp
    SQLParser/test/SqlParserTest.cpp
    CatalogService/test/CatalogActionTestSupport.h
    CatalogService/test/CreateRootCatalogPageAction/CreatesWritesAndUnpinsRootPageTest.cpp
    CatalogService/test/CreateRootCatalogPageAction/PropagatesAllocationFailureWithoutPinningTest.cpp
    CatalogService/test/CreateRootCatalogPageAction/UnpinsAllocatedPageWhenWriteFailsTest.cpp
    CatalogService/test/CreateTableAction/CreatesTableOnNewPageWhenTailHasNoSpaceTest.cpp
    CatalogService/test/CreateTableAction/CreatesTableWithSchemaTest.cpp
    CatalogService/test/CreateTableAction/PropagatesFetchFailureWithoutLeakingPinsTest.cpp
    CatalogService/test/CreateTableAction/RejectsCyclicCatalogChainWithoutLeakingPinsTest.cpp
    CatalogService/test/CreateTableAction/RejectsDuplicateTableNameTest.cpp
    CatalogService/test/CreateTableAction/RejectsEmptyTableNameTest.cpp
    CatalogService/test/CreateTableAction/ReleasesTailPageWhenAllocationFailsTest.cpp
    CatalogService/test/CreateTableAction/UnpinsPageWhenTableWriteFailsTest.cpp
    CatalogService/test/DropTableAction/PropagatesFetchFailureWithoutPinningTest.cpp
    CatalogService/test/DropTableAction/RejectsCyclicCatalogChainWithoutLeakingPinsTest.cpp
    CatalogService/test/DropTableAction/RemovesMatchingTableAndPersistsPageTest.cpp
    CatalogService/test/DropTableAction/ThrowsWhenTableDoesNotExistTest.cpp
    CatalogService/test/DropTableAction/UnpinsPageWhenWriteFailsTest.cpp
    CatalogService/test/GetTableAction/FindsTableOnLaterCatalogPageTest.cpp
    CatalogService/test/GetTableAction/PropagatesFetchFailureWithoutPinningTest.cpp
    CatalogService/test/GetTableAction/RejectsCyclicCatalogChainWithoutLeakingPinsTest.cpp
    CatalogService/test/GetTableAction/ReturnsTableByIdAndUnpinsPageTest.cpp
    CatalogService/test/GetTableAction/ThrowsWhenTableIdIsMissingTest.cpp
    CatalogService/test/GetTableByNameAction/FindsTableOnLaterCatalogPageTest.cpp
    CatalogService/test/GetTableByNameAction/PropagatesFetchFailureWithoutPinningTest.cpp
    CatalogService/test/GetTableByNameAction/RejectsCyclicCatalogChainWithoutLeakingPinsTest.cpp
    CatalogService/test/GetTableByNameAction/RejectsEmptyNameTest.cpp
    CatalogService/test/GetTableByNameAction/ReturnsTableByNameAndUnpinsPageTest.cpp
    CatalogService/test/GetTableByNameAction/ThrowsWhenNameIsUnknownTest.cpp
    CatalogService/test/UpdateTableAction/PropagatesFetchFailureWithoutPinningTest.cpp
    CatalogService/test/UpdateTableAction/RejectsCyclicCatalogChainWithoutLeakingPinsTest.cpp
    CatalogService/test/UpdateTableAction/RejectsEmptyTableNameTest.cpp
    CatalogService/test/UpdateTableAction/RejectsNameUsedByDifferentTableOnAnotherPageTest.cpp
    CatalogService/test/UpdateTableAction/RelocatesGrowingTableWhenOriginalPageHasNoSpaceTest.cpp
    CatalogService/test/UpdateTableAction/PreservesOriginalTableWhenRelocationAllocationFailsTest.cpp
    CatalogService/test/UpdateTableAction/PreservesOriginalTableWhenRelocationWriteFailsTest.cpp
    CatalogService/test/UpdateTableAction/ThrowsWhenTableDoesNotExistTest.cpp
    CatalogService/test/UpdateTableAction/UnpinsPageWhenWriteFailsTest.cpp
    CatalogService/test/UpdateTableAction/UpdatesExistingTableAndPersistsPageTest.cpp
)

target_link_libraries(tests
    GTest::gtest
    GTest::gtest_main
)

enable_testing()
include(GoogleTest)
gtest_discover_tests(tests)
