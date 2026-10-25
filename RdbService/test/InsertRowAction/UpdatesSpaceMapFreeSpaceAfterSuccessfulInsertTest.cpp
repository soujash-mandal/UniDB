#include "InsertRowActionTestSupport.h"

#include <gtest/gtest.h>

TEST(InsertRowActionTest, UpdatesSpaceMapFreeSpaceAfterSuccessfulInsert) {
  // Scenario 1: Uses the remaining-space result from the successful insert.
  FakeInsertRootPort root;
  FakeInsertTablePort table;
  table.table = {9, "users", {}, 55};
  FakeInsertUpdateTablePort update;
  FakeInsertCreateSpaceMapPort createSpaceMap;
  FakeInsertFindSpacePort findSpace;
  FakeInsertTupleWithFreeSpacePort createTuple;
  FakeInsertUpdateFreeSpacePort updateFreeSpace;
  createTuple.remainingFreeSpace = 7654;
  RdbService::InsertRowAction action(root, table, update, createSpaceMap,
                                     findSpace, createTuple, updateFreeSpace);

  const auto inserted = action.execute(3, "users", {'x'});

  EXPECT_EQ(inserted.tuplePageId, 81);
  EXPECT_EQ(inserted.slotId, 4);
  EXPECT_EQ(createTuple.calls, 1);
  EXPECT_EQ(createTuple.pageId, 81);
  EXPECT_EQ(updateFreeSpace.calls, 1);
  EXPECT_EQ(updateFreeSpace.spaceMapPageId, 91);
  EXPECT_EQ(updateFreeSpace.tuplePageId, 81);
  EXPECT_EQ(updateFreeSpace.freeSpace, 7654);
}
