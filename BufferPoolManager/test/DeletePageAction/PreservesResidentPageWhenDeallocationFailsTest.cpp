#include "DeletePageActionTestSupport.h"

#include <gtest/gtest.h>

TEST(DeletePageActionTest, PreservesResidentPageWhenDeallocationFails) {
  // Scenario 1: Allocator failure restores the page to resident FIFO tracking.
  BufferPool pool(1);
  FIFOEvictionPolicy policy;
  FakeDeletePagePort deallocate;
  deallocate.fail = true;
  Frame &frame = pool.getFrame(0);
  frame.setPageId(17);
  frame.setOccupied(true);
  policy.RecordAccess(17);
  policy.SetEvictable(17, true);
  DeletePageAction action(pool, policy, deallocate);

  EXPECT_THROW(action.execute(17), std::runtime_error);
  EXPECT_TRUE(frame.isOccupied());
  EXPECT_EQ(frame.getPageId(), 17);
  EXPECT_EQ(policy.Evict().value(), 17);
}
