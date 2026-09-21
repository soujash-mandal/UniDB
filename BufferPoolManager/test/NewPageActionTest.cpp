#include "../actions/NewPageAction.h"
#include "../../EvictionPolicy/FIFOEvictionPolicy.h"

#include <gtest/gtest.h>

#include <stdexcept>

class FakeAllocatePagePort : public AllocatePagePort {
public:
  PageId nextPageId = 1;
  int allocateCount = 0;

  PageId allocatePage() override {
    ++allocateCount;
    return nextPageId++;
  }
};

class NewPageActionTest : public ::testing::Test {
protected:
  FakeAllocatePagePort allocatePagePort;
};

// ============================================================
// Free frame
// ============================================================

TEST_F(NewPageActionTest, UsesFreeFrame) {
  BufferPool bufferPool(1);
  FIFOEvictionPolicy evictionPolicy;

  NewPageAction action(bufferPool, allocatePagePort, evictionPolicy);

  Page &page = action.execute();

  Frame &frame = bufferPool.getFrame(0);

  EXPECT_EQ(&page, &frame.getPage());
  EXPECT_TRUE(frame.isOccupied());
  EXPECT_EQ(frame.getPageId(), 1);
}

// ============================================================
// New page is pinned
// ============================================================

TEST_F(NewPageActionTest, NewPageIsPinned) {
  BufferPool bufferPool(1);
  FIFOEvictionPolicy evictionPolicy;

  NewPageAction action(bufferPool, allocatePagePort, evictionPolicy);

  action.execute();

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);
}

// ============================================================
// New page is dirty
// ============================================================

TEST_F(NewPageActionTest, NewPageIsDirty) {
  BufferPool bufferPool(1);
  FIFOEvictionPolicy evictionPolicy;

  NewPageAction action(bufferPool, allocatePagePort, evictionPolicy);

  action.execute();

  EXPECT_TRUE(bufferPool.getFrame(0).isDirty());
}

// ============================================================
// New page is registered in FIFO
// ============================================================

TEST_F(NewPageActionTest, NewPageIsRegisteredWithEvictionPolicy) {
  BufferPool bufferPool(1);
  FIFOEvictionPolicy evictionPolicy;

  NewPageAction action(bufferPool, allocatePagePort, evictionPolicy);

  action.execute();

  // New page is registered but non-evictable.
  // Therefore Evict() must not return it.
  EXPECT_FALSE(evictionPolicy.Evict().has_value());
}

// ============================================================
// New page is non-evictable
// ============================================================

TEST_F(NewPageActionTest, NewPageIsMarkedNonEvictable) {
  BufferPool bufferPool(1);
  FIFOEvictionPolicy evictionPolicy;

  NewPageAction action(bufferPool, allocatePagePort, evictionPolicy);

  action.execute();

  // The page was registered with FIFO but marked
  // non-evictable.
  EXPECT_FALSE(evictionPolicy.Evict().has_value());
}

// ============================================================
// Allocation happens once
// ============================================================

TEST_F(NewPageActionTest, AllocatesPageExactlyOnce) {
  BufferPool bufferPool(1);
  FIFOEvictionPolicy evictionPolicy;

  NewPageAction action(bufferPool, allocatePagePort, evictionPolicy);

  action.execute();

  EXPECT_EQ(allocatePagePort.allocateCount, 1);
}

// ============================================================
// Full buffer -> Evict() is called
//
// We make page 1 evictable manually.
// ============================================================

TEST_F(NewPageActionTest, CallsEvictWhenBufferIsFull) {
  BufferPool bufferPool(1);
  FIFOEvictionPolicy evictionPolicy;

  // First page.
  NewPageAction firstAction(bufferPool, allocatePagePort, evictionPolicy);

  firstAction.execute();

  // Make page 1 evictable.
  evictionPolicy.SetEvictable(1, true);

  // NewPage must now evict page 1.
  NewPageAction secondAction(bufferPool, allocatePagePort, evictionPolicy);

  secondAction.execute();

  EXPECT_EQ(bufferPool.getFrame(0).getPageId(), 2);
}

// ============================================================
// Full buffer -> victim is replaced
// ============================================================

TEST_F(NewPageActionTest, ReplacesEvictedPage) {
  BufferPool bufferPool(1);
  FIFOEvictionPolicy evictionPolicy;

  NewPageAction firstAction(bufferPool, allocatePagePort, evictionPolicy);

  firstAction.execute();

  EXPECT_EQ(bufferPool.getFrame(0).getPageId(), 1);

  // Page 1 can now be evicted.
  evictionPolicy.SetEvictable(1, true);

  NewPageAction secondAction(bufferPool, allocatePagePort, evictionPolicy);

  secondAction.execute();

  EXPECT_EQ(bufferPool.getFrame(0).getPageId(), 2);
}

// ============================================================
// Replacement page is pinned
// ============================================================

TEST_F(NewPageActionTest, ReplacementPageIsPinned) {
  BufferPool bufferPool(1);
  FIFOEvictionPolicy evictionPolicy;

  NewPageAction firstAction(bufferPool, allocatePagePort, evictionPolicy);

  firstAction.execute();

  // First page must be unpinned before it can be evicted.
  bufferPool.getFrame(0).unpin();

  evictionPolicy.SetEvictable(1, true);

  NewPageAction secondAction(bufferPool, allocatePagePort, evictionPolicy);

  secondAction.execute();

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);
}

// ============================================================
// Replacement page is dirty
// ============================================================

TEST_F(NewPageActionTest, ReplacementPageIsDirty) {
  BufferPool bufferPool(1);
  FIFOEvictionPolicy evictionPolicy;

  NewPageAction firstAction(bufferPool, allocatePagePort, evictionPolicy);

  firstAction.execute();

  bufferPool.getFrame(0).unpin();

  evictionPolicy.SetEvictable(1, true);

  NewPageAction secondAction(bufferPool, allocatePagePort, evictionPolicy);

  secondAction.execute();

  EXPECT_TRUE(bufferPool.getFrame(0).isDirty());
}

// ============================================================
// Replacement page is registered in FIFO
// ============================================================

TEST_F(NewPageActionTest, ReplacementPageIsRegisteredWithEvictionPolicy) {
  BufferPool bufferPool(1);
  FIFOEvictionPolicy evictionPolicy;

  NewPageAction firstAction(bufferPool, allocatePagePort, evictionPolicy);

  firstAction.execute();

  bufferPool.getFrame(0).unpin();

  evictionPolicy.SetEvictable(1, true);

  NewPageAction secondAction(bufferPool, allocatePagePort, evictionPolicy);

  secondAction.execute();

  // New page 2 exists in FIFO.
  // But it is non-evictable.
  //
  // Make page 2 evictable manually.
  evictionPolicy.SetEvictable(2, true);

  auto victim = evictionPolicy.Evict();

  ASSERT_TRUE(victim.has_value());
  EXPECT_EQ(victim.value(), 2);
}

// ============================================================
// Replacement page is non-evictable
// ============================================================

TEST_F(NewPageActionTest, ReplacementPageIsMarkedNonEvictable) {
  BufferPool bufferPool(1);
  FIFOEvictionPolicy evictionPolicy;

  NewPageAction firstAction(bufferPool, allocatePagePort, evictionPolicy);

  firstAction.execute();

  bufferPool.getFrame(0).unpin();

  evictionPolicy.SetEvictable(1, true);

  NewPageAction secondAction(bufferPool, allocatePagePort, evictionPolicy);

  secondAction.execute();

  // If NewPageAction correctly marked page 2 as
  // non-evictable, Evict() should find nothing.
  EXPECT_FALSE(evictionPolicy.Evict().has_value());
}

// ============================================================
// Full buffer + no evictable page -> throw
// ============================================================

TEST_F(NewPageActionTest, ThrowsWhenNoPageIsEvictable) {
  BufferPool bufferPool(1);
  FIFOEvictionPolicy evictionPolicy;

  NewPageAction firstAction(bufferPool, allocatePagePort, evictionPolicy);

  firstAction.execute();

  // Page 1 is pinned and non-evictable.
  EXPECT_THROW(firstAction.execute(), std::runtime_error);
}