#include "InsertRowActionTestSupport.h"

#include <gtest/gtest.h>

// Scenario 1: The exception makes it explicit that the row was committed even
// though refreshing its SpaceMap entry failed.
TEST(InsertRowActionTest, ReportsCommittedLocationWhenSpaceMapUpdateFails) {
  FakeInsertRootPort root;
  FakeInsertTablePort table;
  table.table = {9, "users", {}, 55};
  FakeInsertUpdateTablePort update;
  FakeInsertCreateSpaceMapPort createSpaceMap;
  FakeInsertCalculateRequiredSpacePort calculateRequiredSpace;
  FakeInsertFindSpacePort findSpace;
  FakeInsertTuplePort createTuple;
  FakeInsertReclaimPagePort reclaimPage;
  FakeInsertUpdateFreeSpacePort updateFreeSpace;
  updateFreeSpace.failUpdate = true;
  RdbService::InsertRowAction action(root, table, update, createSpaceMap, reclaimPage,
                                     calculateRequiredSpace, findSpace,
                                     createTuple, updateFreeSpace);

  bool caughtCommittedInsert = false;
  try {
    (void)action.execute(3, "users", {'x'});
  } catch (const RdbService::SpaceMapUpdateFailedAfterInsert &error) {
    caughtCommittedInsert = true;
    EXPECT_EQ(error.getInsertedRowLocation().tuplePageId, 81);
    EXPECT_EQ(error.getInsertedRowLocation().slotId, 4);
    EXPECT_STREQ(error.what(), "SpaceMap update failed");
  }

  EXPECT_TRUE(caughtCommittedInsert);
  EXPECT_EQ(createTuple.calls, 1);
  EXPECT_EQ(updateFreeSpace.calls, 1);
}
