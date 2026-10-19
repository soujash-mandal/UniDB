#include "TestSupport.h"

// Scenario 1: Unpinning the last pin releases the page for eviction.
TEST(UnpinPageActionTest, LastUnpinMakesPageEvictable) {
  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 1);
  UnpinPageAction action(pool, policy);

  EXPECT_NO_THROW(action.execute(7));

  EXPECT_EQ(pool.getFrame(0).getPinCount(), 0);
  EXPECT_EQ(policy.setEvictableCount, 1);
  EXPECT_EQ(policy.lastPageId, 7);
  EXPECT_TRUE(policy.lastEvictable);
}
