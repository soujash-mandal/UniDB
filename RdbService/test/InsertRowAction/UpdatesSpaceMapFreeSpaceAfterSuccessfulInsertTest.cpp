#include "InsertRowActionTestSupport.h"

#include <gtest/gtest.h>

TEST(InsertRowActionTest, UpdatesSpaceMapFreeSpaceAfterSuccessfulInsert) {
  // Scenario 1: The actual remaining free space is stored in the owning
  // SpaceMap entry.
  FakeInsertRootPort root;
  FakeInsertTablePort table;
  table.table = {9, "users", {}, 55};
  FakeInsertUpdateTablePort update;
  FakeInsertCreateSpaceMapPort createSpaceMap;
  FakeInsertFindSpacePort findSpace;
  FakeInsertTuplePort createTuple;
  FakeInsertGetFreeSpacePort getFreeSpace;
  FakeInsertUpdateFreeSpacePort updateFreeSpace;
  RdbService::InsertRowAction action(root, table, update, createSpaceMap,
                                     findSpace, createTuple, getFreeSpace,
                                     updateFreeSpace);

  const auto inserted = action.execute(3, "users", {'x'});

  EXPECT_EQ(inserted.tuplePageId, 81);
  EXPECT_EQ(inserted.slotId, 4);
  EXPECT_EQ(getFreeSpace.calls, 1);
  EXPECT_EQ(getFreeSpace.pageId, 81);
  EXPECT_EQ(updateFreeSpace.calls, 1);
  EXPECT_EQ(updateFreeSpace.spaceMapPageId, 91);
  EXPECT_EQ(updateFreeSpace.tuplePageId, 81);
  EXPECT_EQ(updateFreeSpace.freeSpace, 1234);
}
