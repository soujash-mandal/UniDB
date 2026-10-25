#include "InsertRowActionTestSupport.h"

#include <gtest/gtest.h>
#include <stdexcept>

TEST(InsertRowActionTest, PropagatesPageDiscoveryFailure) {
  // Scenario 1: Discovery errors propagate without tuple insertion.
  FakeInsertRootPort root;
  FakeInsertTablePort table;
  table.table = {9, "users", {}, 55};
  FakeInsertUpdateTablePort update;
  FakeInsertCreateSpaceMapPort createSpaceMap;
  FakeInsertTuplePort createTuple;
  class FailingSpaceLookup final : public FindPageWithSpacePort {
  public:
    RdbFindPageWithSpaceResult findPageWithSpace(uint32_t, uint32_t) override {
      throw std::runtime_error("space lookup failed");
    }
  } findSpace;
  FakeInsertGetFreeSpacePort getFreeSpace;
  FakeInsertUpdateFreeSpacePort updateFreeSpace;
  RdbService::InsertRowAction action(root, table, update, createSpaceMap,
                                     findSpace, createTuple, getFreeSpace,
                                     updateFreeSpace);

  EXPECT_THROW(action.execute(3, "users", {'x'}), std::runtime_error);
  EXPECT_EQ(createTuple.calls, 0);
}
