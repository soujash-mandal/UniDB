#include "TestSupport.h"

// Scenario 5: If making the page evictable fails, its final pin is retained
// so the caller can retry without policy/frame divergence.
TEST(UnpinPageActionTest, PolicyFailurePreservesFinalPinForRetry) {
  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 1);
  UnpinPageAction action(pool, policy);
  policy.shouldThrowOnEvictable = true;

  EXPECT_THROW(action.execute(7), std::runtime_error);
  EXPECT_EQ(pool.getFrame(0).getPinCount(), 1);

  EXPECT_NO_THROW(action.execute(7));
  EXPECT_EQ(pool.getFrame(0).getPinCount(), 0);
  EXPECT_TRUE(policy.lastEvictable);
}
