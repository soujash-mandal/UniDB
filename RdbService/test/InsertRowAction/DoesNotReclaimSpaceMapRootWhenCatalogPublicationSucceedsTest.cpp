#include "InsertRowActionTestSupport.h"

#include <gtest/gtest.h>

TEST(InsertRowActionTest, DoesNotReclaimSpaceMapRootWhenCatalogPublicationSucceeds) {
  // Scenario 1: A successfully published root remains allocated and is reused.
  FakeInsertRootPort root;
  FakeInsertTablePort table;
  table.table = {9, "users", {}, std::numeric_limits<uint32_t>::max()};
  FakeInsertUpdateTablePort update;
  FakeInsertCreateSpaceMapPort createSpaceMap;
  FakeInsertReclaimPagePort reclaimPage;
  FakeInsertCalculateRequiredSpacePort calculateRequiredSpace;
  FakeInsertFindSpacePort findSpace;
  FakeInsertTuplePort createTuple;
  FakeInsertUpdateFreeSpacePort updateFreeSpace;
  RdbService::InsertRowAction action(
      root, table, update, createSpaceMap, reclaimPage, calculateRequiredSpace,
      findSpace, createTuple, updateFreeSpace);

  const auto inserted = action.execute(3, "users", {'x'});

  EXPECT_EQ(createSpaceMap.calls, 1);
  EXPECT_EQ(update.calls, 1);
  EXPECT_EQ(reclaimPage.calls, 0);
  EXPECT_EQ(inserted.tuplePageId, 81);
}
