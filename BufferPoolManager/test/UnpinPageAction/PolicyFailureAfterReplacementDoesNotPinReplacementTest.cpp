#include "TestSupport.h"

// Scenario 8: A policy failure must leave the original final pin intact.
// Replacement cannot occur inside the policy callback because UnpinPageAction
// holds the shared buffer-pool mutex until the policy update completes.
TEST(UnpinPageActionTest, PolicyFailureDoesNotChangePageOrPinCount) {
  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 1);
  UnpinPageAction unpinAction(pool, policy);
  Frame &frame = pool.getFrame(0);
  uint32_t pinCountDuringCallback = 0;
  policy.shouldThrowAfterCallback = true;
  policy.onMakeEvictable = [&]() {
    EXPECT_EQ(frame.getPageId(), 7);
    pinCountDuringCallback = frame.getPinCount();
  };

  EXPECT_THROW(unpinAction.execute(7), std::runtime_error);

  EXPECT_EQ(pinCountDuringCallback, 1);
  EXPECT_EQ(frame.getPageId(), 7);
  EXPECT_EQ(frame.getPinCount(), 1);
}
