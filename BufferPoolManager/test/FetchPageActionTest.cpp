#include "../actions/FetchPageAction.h"

#include <gtest/gtest.h>

#include <stdexcept>

#include "../../EvictionPolicy/FIFOEvictionPolicy.h"

class FakeReadPagePort : public ReadPagePort {
public:
  int readCount = 0;
  BufferPoolPageId lastPageId = 0;
  BufferPoolPage pageToReturn;
  bool shouldThrow = false;

  BufferPoolPage readDiskPage(BufferPoolPageId pageId) override {
    ++readCount;
    lastPageId = pageId;
    if (shouldThrow) {
      throw std::runtime_error("simulated disk read failure");
    }
    return pageToReturn;
  }
};

class FakeWritePagePort : public WritePagePort {
public:
  int writeCount = 0;
  BufferPoolPageId lastPageId = 0;
  bool shouldThrow = false;

  void writePage(BufferPoolPageId pageId, BufferPoolPage) override {
    ++writeCount;
    lastPageId = pageId;
    if (shouldThrow) {
      throw std::runtime_error("simulated disk write failure");
    }
  }
};

class FakeEvictionPolicy : public EvictionPolicy {
public:
  int recordAccessCount = 0;
  int setEvictableCount = 0;
  int removeCount = 0;
  int evictCount = 0;

  BufferPoolPageId lastRecordedPageId = 0;
  BufferPoolPageId lastEvictablePageId = 0;
  BufferPoolPageId lastRemovedPageId = 0;
  bool lastEvictableValue = false;
  std::optional<BufferPoolPageId> victim;

  void RecordAccess(BufferPoolPageId pageId) override {
    ++recordAccessCount;
    lastRecordedPageId = pageId;
  }

  void SetEvictable(BufferPoolPageId pageId, bool evictable) override {
    ++setEvictableCount;
    lastEvictablePageId = pageId;
    lastEvictableValue = evictable;
  }

  std::optional<BufferPoolPageId> Evict() override {
    ++evictCount;
    return victim;
  }

  void Remove(BufferPoolPageId pageId) override {
    ++removeCount;
    lastRemovedPageId = pageId;
  }
};

// Scenario 1: A page fetches into an empty frame with its bytes and initial frame/policy state intact.
TEST(FetchPageActionTest, FetchesPageIntoEmptyFrame) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FakeEvictionPolicy evictionPolicy;
  const BufferPoolPageId pageId = 10;
  readPort.pageToReturn.data()[0] = 'A';

  FetchPageAction action(bufferPool, readPort, writePort, evictionPolicy);
  BufferPoolPage page = action.execute(pageId);

  EXPECT_EQ(readPort.readCount, 1);
  EXPECT_EQ(readPort.lastPageId, pageId);
  Frame &frame = bufferPool.getFrame(0);
  EXPECT_TRUE(frame.isOccupied());
  EXPECT_EQ(frame.getPageId(), pageId);
  EXPECT_EQ(frame.getPinCount(), 1);
  EXPECT_FALSE(frame.isDirty());
  EXPECT_EQ(page.data()[0], 'A');
  EXPECT_EQ(evictionPolicy.recordAccessCount, 1);
  EXPECT_EQ(evictionPolicy.lastRecordedPageId, pageId);
  EXPECT_EQ(evictionPolicy.setEvictableCount, 1);
  EXPECT_FALSE(evictionPolicy.lastEvictableValue);
}

// Scenario 2: Fetching a cached page reuses its frame, avoids another disk read, and increments its pin count.
TEST(FetchPageActionTest, FetchingCachedPageDoesNotReadAgain) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FakeEvictionPolicy evictionPolicy;
  readPort.pageToReturn.data()[0] = 'A';

  FetchPageAction action(bufferPool, readPort, writePort, evictionPolicy);
  action.execute(10);
  action.execute(10);

  Frame &frame = bufferPool.getFrame(0);
  EXPECT_EQ(readPort.readCount, 1);
  EXPECT_EQ(frame.getPinCount(), 2);
  EXPECT_EQ(frame.getPageId(), 10);
}

// Scenario 3: A clean evictable victim is replaced without writing it, and policy/frame mappings move to the new page.
TEST(FetchPageActionTest, EvictsCleanPageWhenBufferPoolIsFull) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FakeEvictionPolicy evictionPolicy;
  readPort.pageToReturn.data()[0] = 'A';

  FetchPageAction action(bufferPool, readPort, writePort, evictionPolicy);
  action.execute(1);
  Frame &frame = bufferPool.getFrame(0);
  frame.unpin();
  evictionPolicy.victim = 1;
  readPort.pageToReturn.data()[0] = 'B';

  BufferPoolPage page = action.execute(2);

  EXPECT_EQ(readPort.readCount, 2);
  EXPECT_EQ(writePort.writeCount, 0);
  EXPECT_EQ(evictionPolicy.evictCount, 1);
  EXPECT_EQ(evictionPolicy.removeCount, 1);
  EXPECT_EQ(evictionPolicy.lastRemovedPageId, 1);
  EXPECT_EQ(frame.getPageId(), 2);
  EXPECT_EQ(frame.getPinCount(), 1);
  EXPECT_FALSE(frame.isDirty());
  EXPECT_EQ(page.data()[0], 'B');
}

// Scenario 4: Dirty victim contents are written before the frame is reused for another PageId.
TEST(FetchPageActionTest, WritesDirtyVictimBeforeEviction) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FakeEvictionPolicy evictionPolicy;
  FetchPageAction action(bufferPool, readPort, writePort, evictionPolicy);

  action.execute(1);
  Frame &frame = bufferPool.getFrame(0);
  frame.setDirty(true);
  frame.unpin();
  evictionPolicy.victim = 1;

  action.execute(2);

  EXPECT_EQ(writePort.writeCount, 1);
  EXPECT_EQ(writePort.lastPageId, 1);
  EXPECT_EQ(frame.getPageId(), 2);
  EXPECT_FALSE(frame.isDirty());
}

// Scenario 5: If every frame is pinned, fetch fails without replacing the pinned page.
TEST(FetchPageActionTest, ThrowsWhenNoPageIsEvictable) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FakeEvictionPolicy evictionPolicy;
  FetchPageAction action(bufferPool, readPort, writePort, evictionPolicy);
  action.execute(1);
  evictionPolicy.victim = std::nullopt;

  EXPECT_THROW(action.execute(2), std::runtime_error);
  EXPECT_EQ(bufferPool.getFrame(0).getPageId(), 1);
  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);
  EXPECT_EQ(readPort.readCount, 1);
}

// Scenario 6: A policy returning a PageId not resident in the pool fails without changing the resident frame.
TEST(FetchPageActionTest, ThrowsWhenEvictionPolicyReturnsUnknownPage) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FakeEvictionPolicy evictionPolicy;
  FetchPageAction action(bufferPool, readPort, writePort, evictionPolicy);
  action.execute(1);
  bufferPool.getFrame(0).unpin();
  evictionPolicy.victim = 999;

  EXPECT_THROW(action.execute(2), std::runtime_error);
  EXPECT_EQ(bufferPool.getFrame(0).getPageId(), 1);
  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 0);
}

// Scenario 7: Failed dirty-victim writeback keeps the original page dirty and restores it to the eviction policy for retry.
TEST(FetchPageActionTest, FailedDirtyVictimWriteCanBeRetried) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FIFOEvictionPolicy evictionPolicy;
  writePort.shouldThrow = true;
  readPort.pageToReturn.data()[0] = 'A';

  FetchPageAction action(bufferPool, readPort, writePort, evictionPolicy);
  action.execute(1);
  Frame &frame = bufferPool.getFrame(0);
  frame.setDirty(true);
  frame.unpin();
  evictionPolicy.SetEvictable(1, true);

  EXPECT_THROW(action.execute(2), std::runtime_error);
  EXPECT_EQ(frame.getPageId(), 1);
  EXPECT_TRUE(frame.isOccupied());
  EXPECT_TRUE(frame.isDirty());
  EXPECT_EQ(frame.getPinCount(), 0);

  writePort.shouldThrow = false;
  readPort.pageToReturn.data()[0] = 'B';
  EXPECT_NO_THROW(action.execute(2));
  EXPECT_EQ(frame.getPageId(), 2);
  EXPECT_EQ(frame.getPinCount(), 1);
  EXPECT_EQ(readPort.readCount, 2);
  EXPECT_EQ(writePort.writeCount, 2);
}

// Scenario 8: Failed read of the requested page leaves the old page resident and makes it evictable again for a retry.
TEST(FetchPageActionTest, FailedReadRestoresVictimPolicyStateForRetry) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FIFOEvictionPolicy evictionPolicy;
  readPort.pageToReturn.data()[0] = 'A';

  FetchPageAction action(bufferPool, readPort, writePort, evictionPolicy);
  action.execute(1);
  Frame &frame = bufferPool.getFrame(0);
  frame.setDirty(true);
  frame.unpin();
  evictionPolicy.SetEvictable(1, true);
  readPort.shouldThrow = true;

  EXPECT_THROW(action.execute(2), std::runtime_error);
  EXPECT_EQ(frame.getPageId(), 1);
  EXPECT_TRUE(frame.isOccupied());
  EXPECT_FALSE(frame.isDirty()); // The dirty page was successfully written before the read failed.
  EXPECT_EQ(frame.getPinCount(), 0);

  readPort.shouldThrow = false;
  readPort.pageToReturn.data()[0] = 'B';
  EXPECT_NO_THROW(action.execute(2));
  EXPECT_EQ(frame.getPageId(), 2);
  EXPECT_EQ(frame.getPinCount(), 1);
  EXPECT_EQ(readPort.readCount, 3);
  EXPECT_EQ(writePort.writeCount, 1);
}

// Scenario 9: A failed read into an empty frame leaves the frame unoccupied and does not register a phantom page.
TEST(FetchPageActionTest, FailedReadIntoEmptyFrameLeavesFrameUnoccupied) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FakeEvictionPolicy evictionPolicy;
  readPort.shouldThrow = true;
  FetchPageAction action(bufferPool, readPort, writePort, evictionPolicy);

  EXPECT_THROW(action.execute(3), std::runtime_error);

  EXPECT_FALSE(bufferPool.getFrame(0).isOccupied());
  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 0);
  EXPECT_EQ(evictionPolicy.recordAccessCount, 0);
}

// Scenario 10: A zero-sized pool fails a fetch cleanly without attempting disk I/O or mutating frame state.
TEST(FetchPageActionTest, ZeroSizedPoolFailsCleanly) {
  BufferPool bufferPool(0);
  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FakeEvictionPolicy evictionPolicy;
  FetchPageAction action(bufferPool, readPort, writePort, evictionPolicy);

  EXPECT_THROW(action.execute(3), std::runtime_error);
  EXPECT_EQ(readPort.readCount, 0);
  EXPECT_EQ(evictionPolicy.recordAccessCount, 0);
}
