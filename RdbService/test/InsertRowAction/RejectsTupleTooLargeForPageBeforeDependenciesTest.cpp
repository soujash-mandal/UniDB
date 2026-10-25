#include "InsertRowActionTestSupport.h"

#include <gtest/gtest.h>
#include <stdexcept>

TEST(InsertRowActionTest, RejectsTupleTooLargeForPageBeforeDependencies) {
  // Scenario 1: A row larger than one tuple page is rejected before storage
  // changes.
  FakeInsertRootPort root;
  FakeInsertTablePort table;
  FakeInsertUpdateTablePort update;
  FakeInsertCreateSpaceMapPort createSpaceMap;
  FakeInsertFindSpacePort findSpace;
  FakeInsertTuplePort createTuple;
  FakeInsertGetFreeSpacePort getFreeSpace;
  FakeInsertUpdateFreeSpacePort updateFreeSpace;
  RdbService::InsertRowAction action(root, table, update, createSpaceMap,
                                     findSpace, createTuple, getFreeSpace,
                                     updateFreeSpace);
  const std::vector<char> oversizedTuple(8177, 'x');

  EXPECT_THROW(action.execute(0, "users", oversizedTuple),
               std::invalid_argument);
  EXPECT_EQ(root.calls, 0);
  EXPECT_EQ(createSpaceMap.calls, 0);
  EXPECT_EQ(createTuple.calls, 0);
}
