#include "../actions/FetchPageAction.h"
#include "../../EvictionPolicy/FIFOEvictionPolicy.h"

#include <gtest/gtest.h>

#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

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
  bool throwNextRecordAccess = false;
  bool throwNextSetEvictable = false;
  std::optional<BufferPoolPageId> victim;
  std::vector<BufferPoolPageId> accessedPageIds;
  std::vector<std::pair<BufferPoolPageId, bool>> evictableCalls;

  void RecordAccess(BufferPoolPageId pageId) override {
    ++recordAccessCount;
    lastRecordedPageId = pageId;
    if (throwNextRecordAccess) {
      throwNextRecordAccess = false;
      throw std::runtime_error("simulated RecordAccess failure");
    }
    accessedPageIds.push_back(pageId);
  }

  void SetEvictable(BufferPoolPageId pageId, bool evictable) override {
    ++setEvictableCount;
    lastEvictablePageId = pageId;
    lastEvictableValue = evictable;
    if (throwNextSetEvictable) {
      throwNextSetEvictable = false;
      throw std::runtime_error("simulated SetEvictable failure");
    }
    evictableCalls.emplace_back(pageId, evictable);
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

// Scenario 1: A page fetches into an empty frame with its bytes and initial
// frame/policy state intact.
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

// Scenario 2: Fetching a cached page reuses its frame, avoids another disk
// read, and increments its pin count.
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

// Scenario 3: A clean evictable victim is replaced without writing it, and
// policy/frame mappings move to the new page.
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

// Scenario 4: Dirty victim contents are written before the frame is reused
// for another PageId.
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

// Scenario 5: If every frame is pinned, fetch fails without replacing the
// pinned page.
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

// Scenario 6: A policy returning a PageId not resident in the pool fails
// without changing the resident frame.
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

// Scenario 7: Failed dirty-victim writeback keeps the original page dirty and
// restores it to the eviction policy for retry.
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

// Scenario 8: Failed read of the requested page leaves the old page resident
// and makes it evictable again for a retry.
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
  // The dirty page was successfully written before the read failed.
  EXPECT_FALSE(frame.isDirty());
  EXPECT_EQ(frame.getPinCount(), 0);

  readPort.shouldThrow = false;
  readPort.pageToReturn.data()[0] = 'B';
  EXPECT_NO_THROW(action.execute(2));
  EXPECT_EQ(frame.getPageId(), 2);
  EXPECT_EQ(frame.getPinCount(), 1);
  EXPECT_EQ(readPort.readCount, 3);
  EXPECT_EQ(writePort.writeCount, 1);
}

// Scenario 9: A failed read into an empty frame leaves the frame unoccupied
// and does not register a phantom page.
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

// Scenario 10: A zero-sized pool fails a fetch cleanly without attempting
// disk I/O or mutating frame state.
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

// Scenario 11: A cached-fetch policy failure must not leak a pin.
TEST(FetchPageActionTest, PolicyFailureOnCachedFetchDoesNotLeakPin) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FakeEvictionPolicy evictionPolicy;
  FetchPageAction action(bufferPool, readPort, writePort, evictionPolicy);

  action.execute(10);
  Frame &frame = bufferPool.getFrame(0);
  frame.unpin();
  evictionPolicy.throwNextSetEvictable = true;

  EXPECT_THROW(action.execute(10), std::runtime_error);
  EXPECT_EQ(frame.getPinCount(), 0);
  EXPECT_TRUE(frame.isOccupied());
  EXPECT_EQ(frame.getPageId(), 10);
  EXPECT_EQ(evictionPolicy.setEvictableCount, 3);
  EXPECT_EQ(evictionPolicy.lastEvictablePageId, 10);
  EXPECT_TRUE(evictionPolicy.lastEvictableValue);
}

// Scenario 12: An eviction-policy failure while loading an empty frame leaves
// the frame available for a later fetch.
TEST(FetchPageActionTest, PolicyFailureOnEmptyFrameLeavesFrameUnoccupied) {
  BufferPool bufferPool(1);
  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FakeEvictionPolicy evictionPolicy;
  evictionPolicy.throwNextRecordAccess = true;
  FetchPageAction action(bufferPool, readPort, writePort, evictionPolicy);

  EXPECT_THROW(action.execute(10), std::runtime_error);
  EXPECT_FALSE(bufferPool.getFrame(0).isOccupied());
  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 0);
  EXPECT_EQ(evictionPolicy.removeCount, 1);
  EXPECT_EQ(evictionPolicy.lastRemovedPageId, 10);

  EXPECT_NO_THROW(action.execute(10));
  EXPECT_TRUE(bufferPool.getFrame(0).isOccupied());
  EXPECT_EQ(bufferPool.getFrame(0).getPinCount(), 1);
}


// Scenario 13: If RecordAccess fails after victim replacement, the old frame
// is restored and the requested fetch can be retried.
TEST(FetchPageActionTest, ReplacementRecordAccessFailureRestoresVictim) {
  BufferPool pool(1);
  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FakeEvictionPolicy policy;
  readPort.pageToReturn.data()[0] = 'A';
  FetchPageAction action(pool, readPort, writePort, policy);
  action.execute(1);
  Frame &frame = pool.getFrame(0);
  frame.unpin();
  policy.victim = 1;
  policy.throwNextRecordAccess = true;
  readPort.pageToReturn.data()[0] = 'B';

  EXPECT_THROW(action.execute(2), std::runtime_error);
  EXPECT_TRUE(frame.isOccupied());
  EXPECT_EQ(frame.getPageId(), 1);
  EXPECT_EQ(frame.getPage().data()[0], 'A');
  EXPECT_EQ(frame.getPinCount(), 0);
  EXPECT_EQ(policy.lastRemovedPageId, 2);
  ASSERT_FALSE(policy.accessedPageIds.empty());
  EXPECT_EQ(policy.accessedPageIds.back(), 1);
  ASSERT_FALSE(policy.evictableCalls.empty());
  EXPECT_EQ(policy.evictableCalls.back(), std::make_pair(BufferPoolPageId{1}, true));

  policy.victim = 1;
  EXPECT_NO_THROW(action.execute(2));
  EXPECT_EQ(frame.getPageId(), 2);
  EXPECT_EQ(frame.getPinCount(), 1);
}

// Scenario 14: If SetEvictable fails after replacement, the frame mapping is
// rolled back and a retry can fetch the requested page.
TEST(FetchPageActionTest, ReplacementSetEvictableFailureRestoresVictim) {
  BufferPool pool(1);
  FakeReadPagePort readPort;
  FakeWritePagePort writePort;
  FakeEvictionPolicy policy;
  readPort.pageToReturn.data()[0] = 'A';
  FetchPageAction action(pool, readPort, writePort, policy);
  action.execute(1);
  Frame &frame = pool.getFrame(0);
  frame.unpin();
  policy.victim = 1;
  policy.throwNextSetEvictable = true;
  readPort.pageToReturn.data()[0] = 'B';

  EXPECT_THROW(action.execute(2), std::runtime_error);
  EXPECT_TRUE(frame.isOccupied());
  EXPECT_EQ(frame.getPageId(), 1);
  EXPECT_EQ(frame.getPage().data()[0], 'A');
  EXPECT_EQ(frame.getPinCount(), 0);
  EXPECT_EQ(policy.lastRemovedPageId, 2);
  ASSERT_FALSE(policy.accessedPageIds.empty());
  EXPECT_EQ(policy.accessedPageIds.back(), 1);
  ASSERT_FALSE(policy.evictableCalls.empty());
  EXPECT_EQ(policy.evictableCalls.back(), std::make_pair(BufferPoolPageId{1}, true));

  policy.victim = 1;
  EXPECT_NO_THROW(action.execute(2));
  EXPECT_EQ(frame.getPageId(), 2);
  EXPECT_EQ(frame.getPinCount(), 1);
}
