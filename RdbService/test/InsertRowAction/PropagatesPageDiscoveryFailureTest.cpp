#include "InsertRowActionTestSupport.h"

#include <gtest/gtest.h>
#include <stdexcept>

TEST(InsertRowActionTest, PropagatesPageDiscoveryFailure) {
  // Scenario 1: A SpaceMap discovery error is forwarded without attempting
  // tuple insertion.
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
  RdbService::InsertRowAction action(root, table, update, createSpaceMap,
                                     findSpace, createTuple);

  EXPECT_THROW(action.execute(3, "users", {'x'}), std::runtime_error);
  EXPECT_EQ(createTuple.calls, 0);
}
