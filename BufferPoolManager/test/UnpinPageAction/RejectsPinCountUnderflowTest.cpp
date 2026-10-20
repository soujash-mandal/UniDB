#include "TestSupport.h"

// Scenario 4: A second unpin after the pin count reaches zero is rejected and
// does not corrupt policy state.
TEST(UnpinPageActionTest, RejectsPinCountUnderflow) {
  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 1);
  UnpinPageAction action(pool, policy);

  action.execute(7);
  ASSERT_EQ(pool.getFrame(0).getPinCount(), 0);
  ASSERT_EQ(policy.setEvictableCount, 1);

  EXPECT_THROW(action.execute(7), std::runtime_error);

  EXPECT_EQ(pool.getFrame(0).getPinCount(), 0);
  EXPECT_EQ(policy.setEvictableCount, 1);
}
