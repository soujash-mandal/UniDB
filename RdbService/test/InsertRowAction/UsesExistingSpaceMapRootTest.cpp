#include "InsertRowActionTestSupport.h"

#include <gtest/gtest.h>

TEST(InsertRowActionTest, UsesExistingSpaceMapRoot) {
  // Scenario 1: Existing table storage is reused without creating or updating its SpaceMap root.
  FakeInsertRootPort root;
  FakeInsertTablePort table;
  table.table = {9, "users", {}, 55};
  FakeInsertUpdateTablePort update;
  FakeInsertCreateSpaceMapPort createSpaceMap;
  FakeInsertFindSpacePort findSpace;
  FakeInsertTuplePort createTuple;
  RdbService::InsertRowAction action(root, table, update, createSpaceMap,
                                     findSpace, createTuple);

  const auto inserted = action.execute(3, "users", {'x'});

  EXPECT_EQ(createSpaceMap.calls, 0);
  EXPECT_EQ(update.calls, 0);
  EXPECT_EQ(findSpace.root, 55);
  EXPECT_EQ(findSpace.requiredSpace, 9);
  EXPECT_EQ(inserted.tuplePageId, 81);
  EXPECT_EQ(inserted.slotId, 4);
}
