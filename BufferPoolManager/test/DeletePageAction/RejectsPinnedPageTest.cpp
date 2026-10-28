#include "DeletePageActionTestSupport.h"

#include <gtest/gtest.h>

TEST(DeletePageActionTest, RejectsPinnedPage) {
  // Scenario 1: A pinned page remains resident and is not deallocated.
  BufferPool pool(1);
  FIFOEvictionPolicy policy;
  FakeDeletePagePort deallocate;
  Frame &frame = pool.getFrame(0);
  frame.setPageId(17);
  frame.setOccupied(true);
  frame.pin();
  policy.RecordAccess(17);
  policy.SetEvictable(17, false);
  DeletePageAction action(pool, policy, deallocate);

  EXPECT_THROW(action.execute(17), std::runtime_error);
  EXPECT_EQ(deallocate.calls, 0);
  EXPECT_TRUE(frame.isOccupied());
  EXPECT_EQ(frame.getPinCount(), 1);
}
