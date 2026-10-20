#include "TestSupport.h"

// Scenario 6: The final pin must remain held while the eviction policy is
// notified. Reentrant replacement from the callback would deadlock because
// actions intentionally share a non-recursive buffer-pool mutex.
TEST(UnpinPageActionTest, FinalPinRemainsHeldDuringPolicyCallback) {
  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 1);
  UnpinPageAction unpinAction(pool, policy);

  uint32_t pinCountDuringCallback = 0;
  policy.onMakeEvictable = [&]() {
    pinCountDuringCallback = pool.getFrame(0).getPinCount();
  };

  unpinAction.execute(7);

  EXPECT_EQ(pinCountDuringCallback, 1);
  EXPECT_EQ(pool.getFrame(0).getPageId(), 7);
  EXPECT_EQ(pool.getFrame(0).getPinCount(), 0);
  EXPECT_EQ(policy.lastPageId, 7);
  EXPECT_TRUE(policy.lastEvictable);
}
