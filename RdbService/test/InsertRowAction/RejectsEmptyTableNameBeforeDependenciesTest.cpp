#include "InsertRowActionTestSupport.h"

#include <gtest/gtest.h>
#include <stdexcept>

TEST(InsertRowActionTest, RejectsEmptyTableNameBeforeDependencies) {
  // Scenario 1: Empty table names fail before storage calls.
  FakeInsertRootPort root;
  FakeInsertTablePort table;
  FakeInsertUpdateTablePort update;
  FakeInsertCreateSpaceMapPort createSpaceMap;
  FakeInsertCalculateRequiredSpacePort calculateRequiredSpace;
  FakeInsertFindSpacePort findSpace;
  FakeInsertTupleWithFreeSpacePort createTuple;
  FakeInsertUpdateFreeSpacePort updateFreeSpace;
  RdbService::InsertRowAction action(root, table, update, createSpaceMap,
                                     calculateRequiredSpace, findSpace,
                                     createTuple, updateFreeSpace);

  EXPECT_THROW(action.execute(0, "", {'x'}), std::invalid_argument);
  EXPECT_EQ(root.calls, 0);
  EXPECT_EQ(table.calls, 0);
  EXPECT_EQ(createTuple.calls, 0);
}
