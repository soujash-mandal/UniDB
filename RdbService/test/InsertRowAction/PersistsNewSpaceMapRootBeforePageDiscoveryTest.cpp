#include "InsertRowActionTestSupport.h"

#include <gtest/gtest.h>
#include <stdexcept>

TEST(InsertRowActionTest, PersistsNewSpaceMapRootBeforePageDiscovery) {
  // Scenario 1: A new root remains catalog-owned when discovery fails.
  FakeInsertRootPort root;
  FakeInsertTablePort table;
  table.table = {9, "users", {}, std::numeric_limits<uint32_t>::max()};
  FakeInsertUpdateTablePort update;
  FakeInsertCreateSpaceMapPort createSpaceMap;
  FakeInsertCalculateRequiredSpacePort calculateRequiredSpace;
  class FailingSpaceLookup final : public FindPageWithSpacePort {
  public:
    RdbFindPageWithSpaceResult findPageWithSpace(uint32_t, uint32_t) override {
      throw std::runtime_error("space lookup failed");
    }
  } findSpace;
  FakeInsertTupleWithFreeSpacePort createTuple;
  FakeInsertUpdateFreeSpacePort updateFreeSpace;
  RdbService::InsertRowAction action(root, table, update, createSpaceMap,
                                     calculateRequiredSpace, findSpace,
                                     createTuple, updateFreeSpace);

  EXPECT_THROW(action.execute(3, "users", {'x'}), std::runtime_error);
  EXPECT_EQ(createSpaceMap.calls, 1);
  EXPECT_EQ(update.calls, 1);
  EXPECT_EQ(update.root, 17);
  EXPECT_EQ(update.spaceMapRoot, 71);
  EXPECT_EQ(createTuple.calls, 0);
  EXPECT_EQ(updateFreeSpace.calls, 0);
}
