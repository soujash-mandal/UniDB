#include "TestSupport.h"

// Scenario 6: Reproduce replacement between SetEvictable(true) and the
// original unpin by invoking the real NewPageAction from the policy callback.
// The replacement page must start with exactly its own pin, not inherit the
// victim page's outstanding pin.
TEST(UnpinPageActionTest, ReplacementDoesNotInheritVictimPin) {
  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 1);
  UnpinPageAction unpinAction(pool, policy);
  UnpinTestAllocatePort allocatePort;
  UnpinTestWritePort writePort;
  NewPageAction newPageAction(pool, allocatePort, writePort, policy);
  policy.victim = 7;

  uint32_t replacementPinCountBeforeOldUnpin = 0;
  policy.onMakeEvictable = [&]() {
    EXPECT_EQ(newPageAction.execute(), 8);
    replacementPinCountBeforeOldUnpin = pool.getFrame(0).getPinCount();
  };

  unpinAction.execute(7);

  EXPECT_EQ(pool.getFrame(0).getPageId(), 8);
  EXPECT_EQ(replacementPinCountBeforeOldUnpin, 1);
  EXPECT_EQ(pool.getFrame(0).getPinCount(), 1);
}
