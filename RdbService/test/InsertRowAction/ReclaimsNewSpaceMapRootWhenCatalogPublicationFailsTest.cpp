#include "InsertRowActionTestSupport.h"

#include <gtest/gtest.h>
#include <stdexcept>

TEST(InsertRowActionTest, ReclaimsNewSpaceMapRootWhenCatalogPublicationFails) {
  // Scenario 1: Catalog publication failure reclaims the newly allocated root.
  FakeInsertRootPort root;
  FakeInsertTablePort table;
  table.table = {9, "users", {}, std::numeric_limits<uint32_t>::max()};
  FakeInsertUpdateTablePort update;
  update.failUpdate = true;
  FakeInsertCreateSpaceMapPort createSpaceMap;
  FakeInsertReclaimPagePort reclaimPage;
  FakeInsertCalculateRequiredSpacePort calculateRequiredSpace;
  FakeInsertFindSpacePort findSpace;
  FakeInsertTuplePort createTuple;
  FakeInsertUpdateFreeSpacePort updateFreeSpace;
  RdbService::InsertRowAction action(root, table, update, createSpaceMap,
                                     reclaimPage, calculateRequiredSpace,
                                     findSpace, createTuple, updateFreeSpace);

  EXPECT_THROW(action.execute(3, "users", {'x'}), std::runtime_error);
  EXPECT_EQ(createSpaceMap.calls, 1);
  EXPECT_EQ(update.calls, 1);
  EXPECT_EQ(reclaimPage.calls, 1);
  EXPECT_EQ(reclaimPage.pageId, createSpaceMap.pageId);
  EXPECT_EQ(findSpace.calls, 0);
  EXPECT_EQ(createTuple.calls, 0);
}
