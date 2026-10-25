#include "InsertRowActionTestSupport.h"

#include <gtest/gtest.h>

TEST(InsertRowActionTest, CreatesSpaceMapRootForFirstInsertedRow) {
  // Scenario 1: First insert creates the SpaceMap root and writes the row.
  FakeInsertRootPort root;
  FakeInsertTablePort table;
  table.table = {9, "users", {}, std::numeric_limits<uint32_t>::max()};
  FakeInsertUpdateTablePort update;
  FakeInsertCreateSpaceMapPort createSpaceMap;
  FakeInsertCalculateRequiredSpacePort calculateRequiredSpace;
  FakeInsertFindSpacePort findSpace;
  FakeInsertTupleWithFreeSpacePort createTuple;
  FakeInsertUpdateFreeSpacePort updateFreeSpace;
  RdbService::InsertRowAction action(root, table, update, createSpaceMap,
                                     calculateRequiredSpace, findSpace,
                                     createTuple, updateFreeSpace);

  const auto inserted = action.execute(3, "users", {'a', 'b'});

  EXPECT_EQ(createSpaceMap.calls, 1);
  EXPECT_EQ(update.calls, 1);
  EXPECT_EQ(update.root, 17);
  EXPECT_EQ(update.spaceMapRoot, 71);
  EXPECT_EQ(findSpace.root, 71);
  EXPECT_EQ(findSpace.requiredSpace, 10);
  EXPECT_EQ(createTuple.pageId, 81);
  EXPECT_EQ(createTuple.bytes, (std::vector<char>{'a', 'b'}));
  EXPECT_EQ(inserted.tuplePageId, 81);
  EXPECT_EQ(inserted.slotId, 4);
  EXPECT_EQ(updateFreeSpace.freeSpace, 1234);
}
