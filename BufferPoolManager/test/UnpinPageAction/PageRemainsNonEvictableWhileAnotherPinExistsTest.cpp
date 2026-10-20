#include "TestSupport.h"

// Scenario 2: A page with multiple pins stays non-evictable until its final
// unpin.
TEST(UnpinPageActionTest, PageRemainsNonEvictableWhileAnotherPinExists) {
  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 2);
  UnpinPageAction action(pool, policy);

  action.execute(7);

  EXPECT_EQ(pool.getFrame(0).getPinCount(), 1);
  EXPECT_EQ(policy.setEvictableCount, 0);

  action.execute(7);

  EXPECT_EQ(pool.getFrame(0).getPinCount(), 0);
  EXPECT_EQ(policy.setEvictableCount, 1);
  EXPECT_TRUE(policy.lastEvictable);
}
