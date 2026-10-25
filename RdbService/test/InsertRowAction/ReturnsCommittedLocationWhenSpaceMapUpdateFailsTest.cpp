#include "InsertRowActionTestSupport.h"

#include <gtest/gtest.h>

// Scenario 1: Report the committed row and refresh failure instead of inviting
// a duplicate retry.
TEST(InsertRowActionTest, ReturnsCommittedLocationWhenSpaceMapUpdateFails) {
  FakeInsertRootPort root;
  FakeInsertTablePort table;
  table.table = {9, "users", {}, 55};
  FakeInsertUpdateTablePort update;
  FakeInsertCreateSpaceMapPort createSpaceMap;
  FakeInsertCalculateRequiredSpacePort calculateRequiredSpace;
  FakeInsertFindSpacePort findSpace;
  FakeInsertTupleWithFreeSpacePort createTuple;
  FakeInsertUpdateFreeSpacePort updateFreeSpace;
  createTuple.remainingFreeSpace = 7654;
  updateFreeSpace.failUpdate = true;
  RdbService::InsertRowAction action(root, table, update, createSpaceMap,
                                     calculateRequiredSpace, findSpace,
                                     createTuple, updateFreeSpace);

  const auto inserted = action.execute(3, "users", {'x'});

  EXPECT_EQ(inserted.tuplePageId, 81);
  EXPECT_EQ(inserted.slotId, 4);
  EXPECT_FALSE(inserted.spaceMapUpdated);
  EXPECT_EQ(inserted.spaceMapUpdateError, "SpaceMap update failed");
  EXPECT_EQ(inserted.spaceMapPageId, 91);
  EXPECT_EQ(inserted.remainingFreeSpace, 7654);
  EXPECT_EQ(createTuple.calls, 1);
  EXPECT_EQ(updateFreeSpace.calls, 1);
}
