#include "InsertRowActionTestSupport.h"

#include <gtest/gtest.h>
#include <stdexcept>

TEST(InsertRowActionTest, RejectsTupleTooLargeForPageBeforeDependencies) {
  // Scenario 1: Oversized rows fail before storage calls.
  FakeInsertRootPort root;
  FakeInsertTablePort table;
  FakeInsertUpdateTablePort update;
  FakeInsertCreateSpaceMapPort createSpaceMap;
  FakeInsertCalculateRequiredSpacePort calculateRequiredSpace;
  FakeInsertFindSpacePort findSpace;
  FakeInsertTuplePort createTuple;
  FakeInsertReclaimPagePort reclaimPage;
  FakeInsertUpdateFreeSpacePort updateFreeSpace;
  RdbService::InsertRowAction action(root, table, update, createSpaceMap,
                                     reclaimPage, calculateRequiredSpace,
                                     findSpace, createTuple, updateFreeSpace);
  const std::vector<char> oversizedTuple(8177, 'x');

  EXPECT_THROW(action.execute(0, "users", oversizedTuple),
               std::invalid_argument);
  EXPECT_EQ(calculateRequiredSpace.calls, 0);
  EXPECT_EQ(root.calls, 0);
  EXPECT_EQ(createSpaceMap.calls, 0);
  EXPECT_EQ(createTuple.calls, 0);
}
