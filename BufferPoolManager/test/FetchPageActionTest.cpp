#include "../actions/FetchPageAction.h"

#include <gtest/gtest.h>

#include <cstring>
#include <stdexcept>

class FakeReadPagePort : public ReadPagePort {
public:
  int readCount = 0;
  BufferPoolPageId lastPageId = 0;

  BufferPoolPage pageToReturn;

  BufferPoolPage readPage(BufferPoolPageId pageId) override {
    ++readCount;
    lastPageId = pageId;
    return pageToReturn;
  }
};

class FakeWritePagePort : public WritePagePort {
public:
  int writeCount = 0;
  BufferPoolPageId lastPageId = 0;

  void writePage(BufferPoolPageId pageId, BufferPoolPage page) override {
    ++writeCount;
    lastPageId = pageId;
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

TEST(FetchPageActionTest, FetchesPageIntoEmptyFrame) {
  BufferPool bufferPool(1);

  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FakeEvictionPolicy evictionPolicy;

  BufferPoolPageId pageId = 10;

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

TEST(FetchPageActionTest, EvictsCleanPageWhenBufferPoolIsFull) {
  BufferPool bufferPool(1);

  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FakeEvictionPolicy evictionPolicy;

  readPort.pageToReturn.data()[0] = 'A';

  FetchPageAction action(bufferPool, readPort, writePort, evictionPolicy);

  // Put page 1 into the buffer.
  action.execute(1);

  // Make page 1 evictable.
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

TEST(FetchPageActionTest, ThrowsWhenNoPageIsEvictable) {
  BufferPool bufferPool(1);

  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FakeEvictionPolicy evictionPolicy;

  FetchPageAction action(bufferPool, readPort, writePort, evictionPolicy);

  action.execute(1);

  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);

  evictionPolicy.victim = std::nullopt;

  EXPECT_THROW(action.execute(2), std::runtime_error);
}

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
}
