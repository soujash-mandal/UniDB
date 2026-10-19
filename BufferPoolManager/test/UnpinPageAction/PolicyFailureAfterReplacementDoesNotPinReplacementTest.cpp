#include "TestSupport.h"

// Scenario 8: If policy update throws after a replacement callback, rollback
// must not pin the replacement page.
TEST(UnpinPageActionTest, PolicyFailureAfterReplacementDoesNotPinReplacement) {
  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 1);
  UnpinPageAction unpinAction(pool, policy);
  Frame &frame = pool.getFrame(0);
  policy.shouldThrowAfterCallback = true;
  policy.onMakeEvictable = [&]() {
    // Simulate a replacement occurring before the policy reports failure.
    frame.setPageId(8);
    frame.pin();
  };

  EXPECT_THROW(unpinAction.execute(7), std::runtime_error);

  EXPECT_EQ(frame.getPageId(), 8);
  EXPECT_EQ(frame.getPinCount(), 1);
}
