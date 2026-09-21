#include <gtest/gtest.h>

#include "../actions/UnpinPageAction.h"
#include "../domain/BufferPool.h"
#include "../../EvictionPolicy/FIFOEvictionPolicy.h"

class UnpinPageActionTest : public ::testing::Test {
protected:
  BufferPool bufferPool{1};
  FIFOEvictionPolicy evictionPolicy;
  UnpinPageAction action{bufferPool, evictionPolicy};

  void setupPage(PageId pageId, uint32_t pinCount = 1) {
    Frame &frame = bufferPool.getFrame(0);

    frame.setPageId(pageId);
    frame.setOccupied(true);

    for (uint32_t i = 0; i < pinCount; ++i) {
      frame.pin();
    }

    evictionPolicy.RecordAccess(pageId);
    evictionPolicy.SetEvictable(pageId, false);
  }
};

// ------------------------------------------------------------
// 1. Unpin decreases pin count
// ------------------------------------------------------------

TEST_F(UnpinPageActionTest, DecreasesPinCount) {
  setupPage(1, 1);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);

  action.execute(1, false);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 0);
}

// ------------------------------------------------------------
// 2. Page remains non-evictable while still pinned
// ------------------------------------------------------------

TEST_F(UnpinPageActionTest, RemainsNonEvictableWhileStillPinned) {
  setupPage(1, 2);

  action.execute(1, false);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);

  // Page should still not be evictable.
  EXPECT_FALSE(evictionPolicy.Evict().has_value());
}

// ------------------------------------------------------------
// 3. Page becomes evictable when pin count reaches zero
// ------------------------------------------------------------

TEST_F(UnpinPageActionTest, BecomesEvictableWhenPinCountReachesZero) {
  setupPage(1, 1);

  action.execute(1, false);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 0);

  // Now FIFO should be able to evict it.
  auto victim = evictionPolicy.Evict();

  ASSERT_TRUE(victim.has_value());
  EXPECT_EQ(victim.value(), 1);
}

// ------------------------------------------------------------
// 4. Unpin with dirty=true marks page dirty
// ------------------------------------------------------------

TEST_F(UnpinPageActionTest, MarksPageDirty) {
  setupPage(1, 1);

  EXPECT_FALSE(bufferPool.getFrame(0).isDirty());

  action.execute(1, true);

  EXPECT_TRUE(bufferPool.getFrame(0).isDirty());
}

// ------------------------------------------------------------
// 5. Unpin with dirty=false does not mark a clean page dirty
// ------------------------------------------------------------

TEST_F(UnpinPageActionTest, DoesNotMarkCleanPageDirty) {
  setupPage(1, 1);

  EXPECT_FALSE(bufferPool.getFrame(0).isDirty());

  action.execute(1, false);

  EXPECT_FALSE(bufferPool.getFrame(0).isDirty());
}

// ------------------------------------------------------------
// 6. Throws when page is not in buffer pool
// ------------------------------------------------------------

TEST_F(UnpinPageActionTest, ThrowsWhenPageNotFound) {
  EXPECT_THROW(action.execute(999, false), std::runtime_error);
}
