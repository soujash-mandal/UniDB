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

    SpaceMapService/domain/SpaceMapPage.cpp
    SpaceMapService/actions/CreateSpaceMapPageAction.cpp
    SpaceMapService/actions/FindPageWithSpaceAction.cpp
    SpaceMapService/actions/GetTablePagesAction.cpp
    SpaceMapService/actions/UpdateFreeSpaceAction.cpp

    TupleService/domain/TuplePage.cpp
    TupleService/adapter/BufferPoolFetchPageAdapter.cpp
    TupleService/adapter/BufferPoolNewPageAdapter.cpp
    TupleService/adapter/BufferPoolUnpinPageAdapter.cpp
    TupleService/adapter/BufferPoolWritePageAdapter.cpp
    TupleService/actions/CreateTuplePageAction.cpp
    TupleService/actions/CreateTupleAction.cpp
    TupleService/actions/CalculateRequiredSpaceAction.cpp
    TupleService/actions/GetTupleAction.cpp
    TupleService/actions/GetAllTuplesAction.cpp
    TupleService/actions/DeleteTupleAction.cpp
    QueryExecutionService/adapter/SqlTupleCodec.cpp

    RdbService/actions/InitializeRdbAction.cpp
    RdbService/actions/CreateTableAction.cpp
    RdbService/actions/GetTableDefinitionAction.cpp
    RdbService/actions/SelectAllRowsAction.cpp
    RdbService/actions/InsertRowAction.cpp
    RdbService/adapter/CreateTupleWithFreeSpaceAdapter.cpp
    RdbService/adapter/CreateRdbMetadataPageAdapter.cpp
    RdbService/adapter/CreateRootCatalogPageAdapter.cpp
    RdbService/adapter/CreateCatalogTableAdapter.cpp
    RdbService/adapter/GetCatalogTableByNameAdapter.cpp
    RdbService/adapter/UpdateCatalogTableAdapter.cpp
    RdbService/adapter/CreateSpaceMapPageAdapter.cpp
    RdbService/adapter/FindPageWithSpaceAdapter.cpp
    RdbService/adapter/GetTablePagesAdapter.cpp
    RdbService/adapter/CreateTuplePageAdapter.cpp
    RdbService/adapter/CreateTupleAdapter.cpp
    RdbService/adapter/CalculateRequiredSpaceAdapter.cpp
    RdbService/adapter/UpdateFreeSpaceAdapter.cpp
    RdbService/adapter/GetTupleAdapter.cpp
    RdbService/adapter/GetAllTuplesAdapter.cpp
    RdbService/adapter/UpdateRdbMetadataPageAdapter.cpp
    RdbService/adapter/AllocateNextTableIdAdapter.cpp
    RdbService/adapter/GetCatalogRootPageIdAdapter.cpp

    SQLParser/SqlLexer.cpp
    SQLParser/SqlParserImpl.cpp
    CatalogService/actions/GetTableByNameAction.cpp
    CatalogService/actions/UpdateTableAction.cpp
)

# Compile production translation units once and share them between the
# server executable and the test executable.
add_library(unidb_core STATIC
    DatabaseStartup.cpp
    ${UNIDB_SOURCES}
)

add_executable(unidb
    main.cpp
)
target_link_libraries(unidb PRIVATE unidb_core)

add_executable(tests

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
    TupleService/test/TuplePage/UsesFullPageCapacityAndSkipsDeletedSlotsTest.cpp
    TupleService/test/TuplePage/RejectsCorruptSlotBoundsDuringScanTest.cpp
    TupleService/test/TuplePage/RejectsCorruptSlotCountDuringScanTest.cpp
    TupleService/test/TuplePage/MigratesLegacyEightByteHeaderWithoutLosingRowsTest.cpp
    TupleService/test/TuplePage/MigratesLegacyEmptyTupleWithoutLosingItsSlotTest.cpp
    TupleService/test/TuplePage/StoresEmptyTupleAndSkipsDeletedSlotTest.cpp
    TupleService/test/CreateTuplePageAction/AllocatesInitializesAndUnpinsPageTest.cpp
    TupleService/test/CreateTuplePageAction/UnpinsAllocatedPageWhenInitializationWriteFailsTest.cpp
    TupleService/test/CreateTuplePageAction/PropagatesAllocationFailureTest.cpp
    TupleService/test/CreateTupleAction/InsertsTupleWritesPageAndUnpinsTest.cpp
    TupleService/test/CreateTupleAction/RejectsTupleThatCannotFitAndUnpinsTest.cpp
    TupleService/test/CreateTupleAction/UnpinsWhenPageWriteFailsTest.cpp
    TupleService/test/CreateTupleAction/PropagatesFetchFailureTest.cpp
    TupleService/test/CalculateRequiredSpaceAction/ReturnsPayloadPlusSlotEntryBytesTest.cpp
    TupleService/test/CalculateRequiredSpaceAction/RejectsTupleTooLargeForEmptyPageTest.cpp
    TupleService/test/CalculateRequiredSpaceAction/ReturnsMaximumTupleSpaceTest.cpp
    TupleService/test/GetTupleAction/ReturnsTupleBytesAndUnpinsPageTest.cpp
    TupleService/test/GetTupleAction/RejectsOutOfRangeSlotAndUnpinsPageTest.cpp
    TupleService/test/GetTupleAction/RejectsDeletedSlotAndUnpinsPageTest.cpp
    TupleService/test/GetTupleAction/PropagatesFetchFailureWithoutUnpinningTest.cpp
    TupleService/test/GetAllTuplesAction/ReturnsLiveTuplesAndUnpinsPageTest.cpp
    TupleService/test/GetAllTuplesAction/SkipsDeletedTuplesTest.cpp
    TupleService/test/GetAllTuplesAction/ReturnsEmptyForEmptyPageTest.cpp
    TupleService/test/GetAllTuplesAction/DoesNotUnpinWhenFetchFailsTest.cpp
    TupleService/test/GetAllTuplesAction/UnpinsPageWhenScanFailsTest.cpp
    TupleService/test/GetAllTuplesAction/PropagatesUnpinFailureTest.cpp
    TupleService/test/GetAllTuplesAction/GetAllTuplesActionTestSupport.h
    TupleService/test/DeleteTupleAction/DeletesTupleAndPersistsPageTest.cpp
    TupleService/test/DeleteTupleAction/RejectsOutOfRangeSlotAndUnpinsPageTest.cpp
    TupleService/test/DeleteTupleAction/UnpinsWhenPageWriteFailsTest.cpp
    TupleService/test/DeleteTupleAction/PropagatesFetchFailureTest.cpp
    SpaceMapService/test/FindPageWithSpaceAction/ReturnsTuplePageAndOwningSpaceMapPageWhenSpaceIsAvailableTest.cpp
    SpaceMapService/test/FindPageWithSpaceAction/ReturnsOwningSpaceMapPageFoundLaterInChainTest.cpp
    SpaceMapService/test/FindPageWithSpaceAction/ReturnsOwningSpaceMapPageWhenItCreatesAndRegistersTuplePageTest.cpp
    SpaceMapService/test/FindPageWithSpaceAction/LeavesNoAllocationPinAfterExtendingFullSpaceMapChainTest.cpp
    SpaceMapService/test/FindPageWithSpaceAction/FindPageWithSpaceActionTestSupport.h
    SpaceMapService/test/GetTablePagesAction/ReturnsTuplePagesFromSingleSpaceMapPageTest.cpp
    SpaceMapService/test/GetTablePagesAction/TraversesChainedSpaceMapPagesTest.cpp
    SpaceMapService/test/GetTablePagesAction/RejectsCycleWithoutLeakingPinsTest.cpp
    SpaceMapService/test/GetTablePagesAction/RetriesUnpinFailureAndDoesNotLeakPinnedPageTest.cpp
    SpaceMapService/test/GetTablePagesAction/PropagatesFetchFailureWithoutUnpinningMissingPageTest.cpp
    SpaceMapService/test/GetTablePagesAction/ReturnsEmptyInventoryWhenRootPageIdIsInvalidTest.cpp
    SpaceMapService/test/GetTablePagesAction/RejectsCorruptEntryCountAndUnpinsPageTest.cpp
    SpaceMapService/test/GetTablePagesAction/GetTablePagesActionTestSupport.h
    QueryExecutionService/test/SqlTupleCodecTest.cpp
    QueryExecutionService/test/SqlTupleCodec/LittleEndianEncodingTest.cpp
    QueryExecutionService/test/SqlTupleCodec/LittleEndianDecodingTest.cpp
    SQLParser/test/SqlParserTest.cpp
    CatalogService/test/CatalogActionTestSupport.h
    RdbService/test/CreateCatalogTableAdapter/ConvertsRdbColumnsWhenCreatingCatalogTableTest.cpp
    RdbService/test/GetCatalogTableByNameAdapter/ConvertsCatalogTableToRdbDefinitionTest.cpp
    RdbService/test/UpdateCatalogTableAdapter/PreservesExplicitSpaceMapPageIdWhenUpdatingTest.cpp
    RdbService/test/CreateTableAction/CreatesTableWithAllocatedIdTest.cpp
    RdbService/test/CreateTableAction/RejectsEmptyTableNameBeforeDependenciesTest.cpp
    RdbService/test/CreateTableAction/RejectsEmptySchemaBeforeDependenciesTest.cpp
    RdbService/test/CreateTableAction/RejectsDuplicateColumnNamesBeforeDependenciesTest.cpp
    RdbService/test/CreateTableAction/RejectsEmptyColumnNameBeforeDependenciesTest.cpp
    RdbService/test/CreateTableAction/RejectsZeroSizedVarcharBeforeDependenciesTest.cpp
    RdbService/test/GetTableDefinitionAction/ReturnsDefinitionForExistingTableTest.cpp
    RdbService/test/GetTableDefinitionAction/RejectsEmptyTableNameBeforeDependenciesTest.cpp
    RdbService/test/GetTableDefinitionAction/PropagatesLookupFailureTest.cpp
    RdbService/test/SelectAllRowsAction/SelectAllRowsActionTestSupport.h
    RdbService/test/SelectAllRowsAction/ReturnsRowsAcrossAllTablePagesTest.cpp
    RdbService/test/SelectAllRowsAction/ReturnsEmptyForTableWithoutSpaceMapRootTest.cpp
    RdbService/test/SelectAllRowsAction/RejectsEmptyTableNameBeforeDependenciesTest.cpp
    RdbService/test/SelectAllRowsAction/PropagatesPageEnumerationFailureTest.cpp
    RdbService/test/InsertRowAction/RejectsEmptyTableNameBeforeDependenciesTest.cpp
    RdbService/test/InsertRowAction/CreatesSpaceMapRootForFirstInsertedRowTest.cpp
    RdbService/test/InsertRowAction/UsesExistingSpaceMapRootTest.cpp
    RdbService/test/InsertRowAction/PropagatesPageDiscoveryFailureTest.cpp
    RdbService/test/InsertRowAction/PersistsNewSpaceMapRootBeforePageDiscoveryTest.cpp
    RdbService/test/InsertRowAction/RejectsTupleTooLargeForPageBeforeDependenciesTest.cpp
    RdbService/test/InsertRowAction/ReturnsCommittedLocationWhenSpaceMapUpdateFailsTest.cpp
    RdbService/test/CreateSpaceMapPageAdapter/DelegatesPageCreationTest.cpp
    RdbService/test/FindPageWithSpaceAdapter/DelegatesPageDiscoveryTest.cpp
    RdbService/test/GetTablePagesAdapter/DelegatesTablePageEnumerationTest.cpp
    RdbService/test/CreateTuplePageAdapter/DelegatesTuplePageCreationTest.cpp
    RdbService/test/CreateTupleAdapter/DelegatesTupleInsertionTest.cpp
    RdbService/test/CalculateRequiredSpaceAdapter/DelegatesRequiredSpaceCalculationTest.cpp
    RdbService/test/UpdateFreeSpaceAdapter/DelegatesFreeSpaceUpdateTest.cpp
    RdbService/test/InsertRowAction/UpdatesSpaceMapFreeSpaceAfterSuccessfulInsertTest.cpp
    RdbService/test/GetTupleAdapter/DelegatesTupleLookupTest.cpp
    RdbService/test/GetAllTuplesAdapter/DelegatesTupleScanTest.cpp
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
    PRIVATE
    unidb_core
    GTest::gtest
    GTest::gtest_main
)

enable_testing()
include(GoogleTest)
gtest_discover_tests(tests)
