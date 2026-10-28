#include "InsertRowActionTestSupport.h"

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

TEST(InsertRowActionTest, PreservesCatalogExceptionWhenRootReclamationFails) {
  // Scenario 1: Cleanup failure must not mask the original Catalog error.
  FakeInsertRootPort root;
  FakeInsertTablePort table;
  table.table = {9, "users", {}, std::numeric_limits<uint32_t>::max()};
  FakeInsertUpdateTablePort update;
  update.failUpdate = true;
  update.failureMessage = "catalog publication failed";
  FakeInsertCreateSpaceMapPort createSpaceMap;
  FakeInsertReclaimPagePort reclaimPage;
  reclaimPage.failReclaim = true;
  FakeInsertCalculateRequiredSpacePort calculateRequiredSpace;
  FakeInsertFindSpacePort findSpace;
  FakeInsertTuplePort createTuple;
  FakeInsertUpdateFreeSpacePort updateFreeSpace;
  RdbService::InsertRowAction action(root, table, update, createSpaceMap,
                                     reclaimPage, calculateRequiredSpace,
                                     findSpace, createTuple, updateFreeSpace);

  try {
    action.execute(3, "users", {'x'});
    FAIL() << "Expected catalog publication to fail";
  } catch (const std::runtime_error &error) {
    EXPECT_EQ(std::string(error.what()), "catalog publication failed");
  }
  EXPECT_EQ(reclaimPage.calls, 1);
  EXPECT_EQ(reclaimPage.pageId, createSpaceMap.pageId);
  EXPECT_EQ(findSpace.calls, 0);
}
