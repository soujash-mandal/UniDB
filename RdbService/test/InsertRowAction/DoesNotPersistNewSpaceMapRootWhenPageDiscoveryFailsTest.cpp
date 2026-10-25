#include "InsertRowActionTestSupport.h"

#include <gtest/gtest.h>
#include <stdexcept>

TEST(InsertRowActionTest, DoesNotPersistNewSpaceMapRootWhenPageDiscoveryFails) {
  // Scenario 1: Failed page discovery must not persist a newly allocated root.
  FakeInsertRootPort root;
  FakeInsertTablePort table;
  table.table = {9, "users", {}, std::numeric_limits<uint32_t>::max()};
  FakeInsertUpdateTablePort update;
  FakeInsertCreateSpaceMapPort createSpaceMap;
  class FailingSpaceLookup final : public FindPageWithSpacePort {
  public:
    RdbFindPageWithSpaceResult findPageWithSpace(uint32_t, uint32_t) override {
      throw std::runtime_error("space lookup failed");
    }
  } findSpace;
  FakeInsertTuplePort createTuple;
  FakeInsertGetFreeSpacePort getFreeSpace;
  FakeInsertUpdateFreeSpacePort updateFreeSpace;
  RdbService::InsertRowAction action(root, table, update, createSpaceMap,
                                     findSpace, createTuple, getFreeSpace,
                                     updateFreeSpace);

  EXPECT_THROW(action.execute(3, "users", {'x'}), std::runtime_error);
  EXPECT_EQ(createSpaceMap.calls, 1);
  EXPECT_EQ(update.calls, 0);
  EXPECT_EQ(createTuple.calls, 0);
}
