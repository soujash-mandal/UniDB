#include "TestSupport.h"

// Scenario 3: Unpinning a page that is not resident fails without changing
// another frame.
TEST(UnpinPageActionTest, UnknownPageThrowsWithoutChangingResidentPage) {
  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 1);
  UnpinPageAction action(pool, policy);

  EXPECT_THROW(action.execute(99), std::runtime_error);

  EXPECT_EQ(pool.getFrame(0).getPageId(), 7);
  EXPECT_EQ(pool.getFrame(0).getPinCount(), 1);
  EXPECT_EQ(policy.setEvictableCount, 0);
}
