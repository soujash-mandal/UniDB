#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../../EvictionPolicy/EvictionPolicy.h"
#include "../../core/Page.h"
#include "../../core/PageId.h"

#include "../actions/FetchPageAction.h"
#include "../domain/BufferPool.h"
#include "../domain/Frame.h"
#include "../port/ReadPagePort.h"
#include "../port/WritePagePort.h"

// ============================================================================
// Fake ReadPagePort
// ============================================================================

class FakeReadPagePort : public ReadPagePort {
public:
  struct ReadCall {
    PageId pageId;
    Page *page;
  };

  std::vector<ReadCall> calls;

  void readPage(PageId pageId, Page &page) override {
    calls.push_back({pageId, &page});

    // Make the loaded page distinguishable.
    page.data()[0] = static_cast<char>(pageId);
  }

  int readCount() const { return static_cast<int>(calls.size()); }

  bool wasRead(PageId pageId) const {
    for (const auto &call : calls) {
      if (call.pageId == pageId) {
        return true;
      }
    }

    return false;
  }
};

// ============================================================================
// Fake WritePagePort
// ============================================================================

class FakeWritePagePort : public WritePagePort {
public:
  struct WriteCall {
    PageId pageId;
    const Page *page;
  };

  std::vector<WriteCall> calls;

  void writePage(PageId pageId, const Page &page) override {
    calls.push_back({pageId, &page});
  }

  int writeCount() const { return static_cast<int>(calls.size()); }

  bool wasWritten(PageId pageId) const {
    for (const auto &call : calls) {
      if (call.pageId == pageId) {
        return true;
      }
    }

    return false;
  }
};

// ============================================================================
// Fake EvictionPolicy
// ============================================================================

class FakeEvictionPolicy : public EvictionPolicy {
public:
  std::optional<PageId> victim;

  std::vector<PageId> recordedAccesses;

  std::vector<std::pair<PageId, bool>> evictableChanges;

  std::vector<PageId> removedPages;

  void RecordAccess(PageId pageId) override {
    recordedAccesses.push_back(pageId);
  }

  void SetEvictable(PageId pageId, bool evictable) override {
    evictableChanges.emplace_back(pageId, evictable);
  }

  std::optional<PageId> Evict() override { return victim; }

  void Remove(PageId pageId) override { removedPages.push_back(pageId); }

  int accessCount(PageId pageId) const {
    int count = 0;

    for (PageId id : recordedAccesses) {
      if (id == pageId) {
        ++count;
      }
    }

    return count;
  }

  bool wasMarkedNonEvictable(PageId pageId) const {
    for (const auto &[id, evictable] : evictableChanges) {
      if (id == pageId && !evictable) {
        return true;
      }
    }

    return false;
  }

  bool wasRemoved(PageId pageId) const {
    for (PageId id : removedPages) {
      if (id == pageId) {
        return true;
      }
    }

    return false;
  }
};

// ============================================================================
// Test Fixture
// ============================================================================

class FetchPageActionTest : public ::testing::Test {
protected:
  // Keep the pool small so tests are easy to reason about.
  BufferPool bufferPool{2};

  FakeReadPagePort readPagePort;
  FakeWritePagePort writePagePort;
  FakeEvictionPolicy evictionPolicy;

  std::unique_ptr<FetchPageAction> action;

  void SetUp() override {
    action = std::make_unique<FetchPageAction>(bufferPool, readPagePort,
                                               writePagePort, evictionPolicy);
  }

  // --------------------------------------------------------------------------
  // Fills EVERY frame in the buffer pool.
  //
  // This is important for eviction tests.
  // FetchPageAction only calls Evict() when there are no empty frames.
  // --------------------------------------------------------------------------
  void fillBufferPool() {
    for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {

      Frame &frame = bufferPool.getFrame(frameId);

      frame.setPageId(100 + frameId);
      frame.setOccupied(true);
      frame.setDirty(false);
    }
  }

  // --------------------------------------------------------------------------
  // Fills every frame and puts the requested victim in a specific frame.
  // --------------------------------------------------------------------------
  Frame &fillBufferPoolWithVictim(uint32_t victimFrameId, PageId victimPageId) {
    fillBufferPool();

    Frame &victimFrame = bufferPool.getFrame(victimFrameId);

    victimFrame.setPageId(victimPageId);

    return victimFrame;
  }
};

// ============================================================================
// CACHE HIT
// ============================================================================

TEST_F(FetchPageActionTest, ReturnsExistingPageWithoutReadingFromDisk) {
  Frame &frame = bufferPool.getFrame(0);

  frame.setPageId(42);
  frame.setOccupied(true);

  Page &result = action->execute(42);

  EXPECT_EQ(&result, &frame.getPage());

  EXPECT_EQ(readPagePort.readCount(), 0);
  EXPECT_EQ(writePagePort.writeCount(), 0);
}

TEST_F(FetchPageActionTest, ExistingPageIsPinned) {
  Frame &frame = bufferPool.getFrame(0);

  frame.setPageId(42);
  frame.setOccupied(true);

  EXPECT_EQ(frame.getPinCount(), 0);

  action->execute(42);

  EXPECT_EQ(frame.getPinCount(), 1);
}

TEST_F(FetchPageActionTest, ExistingPageRecordsAccess) {
  Frame &frame = bufferPool.getFrame(0);

  frame.setPageId(42);
  frame.setOccupied(true);

  action->execute(42);

  EXPECT_EQ(evictionPolicy.accessCount(42), 1);
}

TEST_F(FetchPageActionTest, ExistingPageBecomesNonEvictable) {
  Frame &frame = bufferPool.getFrame(0);

  frame.setPageId(42);
  frame.setOccupied(true);

  action->execute(42);

  EXPECT_TRUE(evictionPolicy.wasMarkedNonEvictable(42));
}

TEST_F(FetchPageActionTest, ExistingPageIsReturnedFromCorrectFrame) {
  Frame &frame0 = bufferPool.getFrame(0);
  Frame &frame1 = bufferPool.getFrame(1);

  frame0.setPageId(10);
  frame0.setOccupied(true);

  frame1.setPageId(42);
  frame1.setOccupied(true);

  Page &result = action->execute(42);

  EXPECT_EQ(&result, &frame1.getPage());

  EXPECT_EQ(frame0.getPageId(), 10);
  EXPECT_EQ(frame1.getPageId(), 42);
}

TEST_F(FetchPageActionTest, RepeatedFetchPinsPageEachTime) {
  Frame &frame = bufferPool.getFrame(0);

  frame.setPageId(42);
  frame.setOccupied(true);

  action->execute(42);
  action->execute(42);
  action->execute(42);

  EXPECT_EQ(frame.getPinCount(), 3);
}

TEST_F(FetchPageActionTest, RepeatedFetchRecordsEveryAccess) {
  Frame &frame = bufferPool.getFrame(0);

  frame.setPageId(42);
  frame.setOccupied(true);

  action->execute(42);
  action->execute(42);
  action->execute(42);

  EXPECT_EQ(evictionPolicy.accessCount(42), 3);
}

// ============================================================================
// EMPTY FRAME
// ============================================================================

TEST_F(FetchPageActionTest, LoadsPageIntoEmptyFrame) {
  Page &result = action->execute(42);

  EXPECT_EQ(readPagePort.readCount(), 1);
  EXPECT_TRUE(readPagePort.wasRead(42));

  Frame &frame = bufferPool.getFrame(0);

  EXPECT_EQ(&result, &frame.getPage());

  EXPECT_TRUE(frame.isOccupied());
  EXPECT_EQ(frame.getPageId(), 42);
}

TEST_F(FetchPageActionTest, EmptyFrameIsInitializedCorrectly) {
  action->execute(42);

  Frame &frame = bufferPool.getFrame(0);

  EXPECT_TRUE(frame.isOccupied());
  EXPECT_EQ(frame.getPageId(), 42);
  EXPECT_FALSE(frame.isDirty());
  EXPECT_EQ(frame.getPinCount(), 1);
}

TEST_F(FetchPageActionTest, LoadedPageRecordsAccess) {
  action->execute(42);

  EXPECT_EQ(evictionPolicy.accessCount(42), 1);
}

TEST_F(FetchPageActionTest, LoadedPageBecomesNonEvictable) {
  action->execute(42);

  EXPECT_TRUE(evictionPolicy.wasMarkedNonEvictable(42));
}

// ============================================================================
// FULL BUFFER POOL
// ============================================================================

TEST_F(FetchPageActionTest, ThrowsWhenNoPageCanBeEvicted) {
  fillBufferPool();

  evictionPolicy.victim = std::nullopt;

  EXPECT_THROW(action->execute(42), std::runtime_error);

  EXPECT_EQ(readPagePort.readCount(), 0);
  EXPECT_EQ(writePagePort.writeCount(), 0);
}

TEST_F(FetchPageActionTest, CallsEvictionPolicyWhenBufferPoolIsFull) {
  fillBufferPool();

  evictionPolicy.victim = 100;

  action->execute(42);

  EXPECT_TRUE(evictionPolicy.wasRemoved(100));
}

// ============================================================================
// CLEAN VICTIM
// ============================================================================

TEST_F(FetchPageActionTest, EvictsCleanPageWithoutWritingIt) {
  Frame &victimFrame = fillBufferPoolWithVictim(0, 10);

  victimFrame.setDirty(false);

  evictionPolicy.victim = 10;

  action->execute(42);

  EXPECT_EQ(writePagePort.writeCount(), 0);

  EXPECT_EQ(readPagePort.readCount(), 1);
  EXPECT_TRUE(readPagePort.wasRead(42));

  EXPECT_EQ(victimFrame.getPageId(), 42);
  EXPECT_TRUE(victimFrame.isOccupied());
  EXPECT_FALSE(victimFrame.isDirty());
  EXPECT_EQ(victimFrame.getPinCount(), 1);
}

TEST_F(FetchPageActionTest, CleanVictimRemainsUnwritten) {
  Frame &victimFrame = fillBufferPoolWithVictim(0, 10);

  victimFrame.setDirty(false);

  evictionPolicy.victim = 10;

  action->execute(42);

  EXPECT_FALSE(writePagePort.wasWritten(10));

  EXPECT_EQ(writePagePort.writeCount(), 0);
}

// ============================================================================
// DIRTY VICTIM
// ============================================================================

TEST_F(FetchPageActionTest, DirtyVictimIsWrittenBeforeReuse) {
  Frame &victimFrame = fillBufferPoolWithVictim(0, 10);

  victimFrame.setDirty(true);

  evictionPolicy.victim = 10;

  ASSERT_TRUE(victimFrame.isOccupied());
  ASSERT_EQ(victimFrame.getPageId(), 10);
  ASSERT_TRUE(victimFrame.isDirty());

  action->execute(42);

  EXPECT_EQ(writePagePort.writeCount(), 1);

  EXPECT_TRUE(writePagePort.wasWritten(10));
}

TEST_F(FetchPageActionTest, DirtyVictimIsCleanedBeforeReuse) {
  Frame &victimFrame = fillBufferPoolWithVictim(0, 10);

  victimFrame.setDirty(true);

  evictionPolicy.victim = 10;

  ASSERT_TRUE(victimFrame.isOccupied());
  ASSERT_EQ(victimFrame.getPageId(), 10);
  ASSERT_TRUE(victimFrame.isDirty());

  action->execute(42);

  EXPECT_EQ(writePagePort.writeCount(), 1);

  EXPECT_TRUE(writePagePort.wasWritten(10));

  EXPECT_EQ(victimFrame.getPageId(), 42);

  EXPECT_TRUE(victimFrame.isOccupied());

  EXPECT_FALSE(victimFrame.isDirty());
}

TEST_F(FetchPageActionTest, DirtyVictimIsWrittenExactlyOnce) {
  Frame &victimFrame = fillBufferPoolWithVictim(0, 10);

  victimFrame.setDirty(true);

  evictionPolicy.victim = 10;

  action->execute(42);

  EXPECT_EQ(writePagePort.writeCount(), 1);
}

// ============================================================================
// EVICTION POLICY
// ============================================================================

TEST_F(FetchPageActionTest, RemovesVictimFromEvictionPolicy) {
  fillBufferPoolWithVictim(0, 10);

  evictionPolicy.victim = 10;

  action->execute(42);

  ASSERT_EQ(evictionPolicy.removedPages.size(), 1);

  EXPECT_EQ(evictionPolicy.removedPages[0], 10);
}

TEST_F(FetchPageActionTest, NewPageRecordsAccessAfterEviction) {
  fillBufferPoolWithVictim(0, 10);

  evictionPolicy.victim = 10;

  action->execute(42);

  EXPECT_EQ(evictionPolicy.accessCount(42), 1);
}

TEST_F(FetchPageActionTest, NewPageBecomesNonEvictableAfterEviction) {
  fillBufferPoolWithVictim(0, 10);

  evictionPolicy.victim = 10;

  action->execute(42);

  EXPECT_TRUE(evictionPolicy.wasMarkedNonEvictable(42));
}

// ============================================================================
// FRAME REUSE
// ============================================================================

TEST_F(FetchPageActionTest, RequestedPageReusesVictimFrame) {
  Frame &victimFrame = fillBufferPoolWithVictim(0, 10);

  evictionPolicy.victim = 10;

  Page &result = action->execute(42);

  EXPECT_EQ(&result, &victimFrame.getPage());

  EXPECT_EQ(victimFrame.getPageId(), 42);

  EXPECT_TRUE(victimFrame.isOccupied());
}

TEST_F(FetchPageActionTest, CorrectVictimFrameIsReused) {
  Frame &frame0 = fillBufferPoolWithVictim(0, 10);

  Frame &frame1 = bufferPool.getFrame(1);

  frame1.setPageId(20);

  evictionPolicy.victim = 20;

  action->execute(42);

  EXPECT_EQ(frame0.getPageId(), 10);

  EXPECT_EQ(frame1.getPageId(), 42);
}

TEST_F(FetchPageActionTest, OtherFramesRemainUnchangedWhenVictimIsReused) {
  Frame &victimFrame = fillBufferPoolWithVictim(0, 10);

  Frame &otherFrame = bufferPool.getFrame(1);

  otherFrame.setPageId(20);
  otherFrame.setDirty(true);

  evictionPolicy.victim = 10;

  action->execute(42);

  EXPECT_EQ(victimFrame.getPageId(), 42);

  EXPECT_EQ(otherFrame.getPageId(), 20);

  EXPECT_TRUE(otherFrame.isDirty());
}

// ============================================================================
// NEW PAGE STATE
// ============================================================================

TEST_F(FetchPageActionTest, NewPageIsPinnedAfterEviction) {
  Frame &victimFrame = fillBufferPoolWithVictim(0, 10);

  evictionPolicy.victim = 10;

  action->execute(42);

  EXPECT_EQ(victimFrame.getPageId(), 42);

  EXPECT_EQ(victimFrame.getPinCount(), 1);
}

TEST_F(FetchPageActionTest, NewlyLoadedPageStartsClean) {
  Frame &victimFrame = fillBufferPoolWithVictim(0, 10);

  victimFrame.setDirty(true);

  evictionPolicy.victim = 10;

  action->execute(42);

  EXPECT_EQ(victimFrame.getPageId(), 42);

  EXPECT_TRUE(victimFrame.isOccupied());

  EXPECT_FALSE(victimFrame.isDirty());

  EXPECT_EQ(victimFrame.getPinCount(), 1);
}

TEST_F(FetchPageActionTest, NewlyLoadedPageIsOccupied) {
  Frame &victimFrame = fillBufferPoolWithVictim(0, 10);

  evictionPolicy.victim = 10;

  action->execute(42);

  EXPECT_TRUE(victimFrame.isOccupied());

  EXPECT_EQ(victimFrame.getPageId(), 42);
}

// ============================================================================
// INVALID VICTIM
// ============================================================================

TEST_F(FetchPageActionTest, ThrowsIfVictimIsNotInBufferPool) {
  fillBufferPool();

  evictionPolicy.victim = 999;

  EXPECT_THROW(action->execute(42), std::runtime_error);

  EXPECT_EQ(readPagePort.readCount(), 0);

  EXPECT_EQ(writePagePort.writeCount(), 0);
}

// ============================================================================
// CACHE HIT TAKES PRIORITY OVER EVICTION
// ============================================================================

TEST_F(FetchPageActionTest, ExistingPageIsFoundBeforeEviction) {
  fillBufferPool();

  Frame &existingFrame = bufferPool.getFrame(1);

  existingFrame.setPageId(42);

  evictionPolicy.victim = 100;

  Page &result = action->execute(42);

  EXPECT_EQ(&result, &existingFrame.getPage());

  EXPECT_EQ(readPagePort.readCount(), 0);

  EXPECT_EQ(writePagePort.writeCount(), 0);

  EXPECT_TRUE(evictionPolicy.removedPages.empty());

  EXPECT_EQ(existingFrame.getPinCount(), 1);
}

// ============================================================================
// COMPLETE EVICTION FLOW
// ============================================================================

TEST_F(FetchPageActionTest, CompleteDirtyVictimReplacementFlow) {
  Frame &victimFrame = fillBufferPoolWithVictim(0, 10);

  victimFrame.setDirty(true);

  evictionPolicy.victim = 10;

  ASSERT_TRUE(victimFrame.isOccupied());

  ASSERT_EQ(victimFrame.getPageId(), 10);

  ASSERT_TRUE(victimFrame.isDirty());

  action->execute(42);

  // --------------------------------------------------------------------------
  // 1. Dirty victim was written.
  // --------------------------------------------------------------------------

  ASSERT_EQ(writePagePort.writeCount(), 1);

  EXPECT_TRUE(writePagePort.wasWritten(10));

  // --------------------------------------------------------------------------
  // 2. Victim was removed from eviction policy.
  // --------------------------------------------------------------------------

  ASSERT_EQ(evictionPolicy.removedPages.size(), 1);

  EXPECT_EQ(evictionPolicy.removedPages[0], 10);

  // --------------------------------------------------------------------------
  // 3. New page was read.
  // --------------------------------------------------------------------------

  ASSERT_EQ(readPagePort.readCount(), 1);

  EXPECT_TRUE(readPagePort.wasRead(42));

  // --------------------------------------------------------------------------
  // 4. Same frame was reused.
  // --------------------------------------------------------------------------

  EXPECT_EQ(victimFrame.getPageId(), 42);

  EXPECT_TRUE(victimFrame.isOccupied());

  // --------------------------------------------------------------------------
  // 5. New page starts clean.
  // --------------------------------------------------------------------------

  EXPECT_FALSE(victimFrame.isDirty());

  // --------------------------------------------------------------------------
  // 6. New page is pinned.
  // --------------------------------------------------------------------------

  EXPECT_EQ(victimFrame.getPinCount(), 1);

  // --------------------------------------------------------------------------
  // 7. New page is registered with eviction policy.
  // --------------------------------------------------------------------------

  EXPECT_EQ(evictionPolicy.accessCount(42), 1);

  // --------------------------------------------------------------------------
  // 8. New page is not evictable while pinned.
  // --------------------------------------------------------------------------

  EXPECT_TRUE(evictionPolicy.wasMarkedNonEvictable(42));
}
