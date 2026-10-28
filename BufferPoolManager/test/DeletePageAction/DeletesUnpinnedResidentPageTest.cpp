#include "DeletePageActionTestSupport.h"

#include <gtest/gtest.h>

TEST(DeletePageActionTest, DeletesUnpinnedResidentPage) {
  // Scenario 1: Deleting an unpinned page frees it and invalidates its frame.
  BufferPool pool(1);
  FIFOEvictionPolicy policy;
  FakeDeletePagePort deallocate;
  Frame &frame = pool.getFrame(0);
  frame.setPageId(17);
  frame.setOccupied(true);
  frame.setDirty(true);
  policy.RecordAccess(17);
  policy.SetEvictable(17, true);
  DeletePageAction action(pool, policy, deallocate);

  action.execute(17);

  EXPECT_EQ(deallocate.calls, 1);
  EXPECT_EQ(deallocate.pageId, 17);
  EXPECT_FALSE(frame.isOccupied());
  EXPECT_FALSE(frame.isDirty());
  EXPECT_EQ(policy.Evict().has_value(), false);
}
