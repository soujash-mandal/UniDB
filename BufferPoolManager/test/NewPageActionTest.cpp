#include "../actions/NewPageAction.h"

#include "../../EvictionPolicy/FIFOEvictionPolicy.h"

#include <gtest/gtest.h>

#include <optional>
#include <stdexcept>

class NewPageTestAllocatePort : public AllocatePagePort {
public:
  BufferPoolPageId nextPageId = 2;
  bool shouldThrow = false;

  BufferPoolPageId allocatePage() override {
    if (shouldThrow) {
      throw std::runtime_error("simulated page allocation failure");
    }
    return nextPageId++;
  }
};

class NewPageTestWritePort : public WritePagePort {
public:
  int writeCount = 0;
  BufferPoolPageId writtenPageId = 0;
  char writtenFirstByte = 0;
  bool shouldThrow = false;

  void writePage(BufferPoolPageId pageId, BufferPoolPage page) override {
    ++writeCount;
    writtenPageId = pageId;
    writtenFirstByte = page.data()[0];
    if (shouldThrow) {
      throw std::runtime_error("simulated disk write failure");
    }
  }
};

class NewPageTestEvictionPolicy : public EvictionPolicy {
public:
  std::optional<BufferPoolPageId> victim;
  bool throwNextRecordAccess = false;
  bool throwNextSetEvictable = false;
  void RecordAccess(BufferPoolPageId) override {
    if (throwNextRecordAccess) {
      throwNextRecordAccess = false;
      throw std::runtime_error("simulated RecordAccess failure");
    }
  }
  void SetEvictable(BufferPoolPageId, bool) override {
    if (throwNextSetEvictable) {
      throwNextSetEvictable = false;
      throw std::runtime_error("simulated SetEvictable failure");
    }
  }
  std::optional<BufferPoolPageId> Evict() override { return victim; }
  void Remove(BufferPoolPageId) override {}
};

// Scenario 1: Reusing a dirty victim flushes its old bytes before the new
// page starts zeroed and pinned.
TEST(NewPageActionTest, ReusedFrameStartsWithZeroedPageContents) {
  BufferPool bufferPool(1);
  Frame &frame = bufferPool.getFrame(0);
  BufferPoolPage oldPage;
  oldPage.data()[0] = 'A';
  frame.setPage(oldPage);
  frame.setPageId(1);
  frame.setOccupied(true);
  frame.setDirty(true);

  NewPageTestAllocatePort allocatePort;
  NewPageTestWritePort writePort;
  NewPageTestEvictionPolicy evictionPolicy;
  evictionPolicy.victim = 1;

  NewPageAction action(bufferPool, allocatePort, writePort, evictionPolicy);
  EXPECT_EQ(action.execute(), 2);

  EXPECT_EQ(writePort.writeCount, 1);
  EXPECT_EQ(writePort.writtenPageId, 1);
  EXPECT_EQ(writePort.writtenFirstByte, 'A');
  EXPECT_EQ(frame.getPageId(), 2);
  EXPECT_EQ(frame.getPinCount(), 1);
  EXPECT_TRUE(frame.isDirty());
  EXPECT_EQ(frame.getPage().data()[0], 0);
}

// Scenario 2: An unused frame is initialized with zeroed bytes without
// writing stale, unoccupied frame contents.
TEST(NewPageActionTest, UnoccupiedFrameStartsWithZeroedPageContents) {
  BufferPool bufferPool(1);
  Frame &frame = bufferPool.getFrame(0);
  BufferPoolPage stalePage;
  stalePage.data()[0] = 'X';
  frame.setPage(stalePage);

  NewPageTestAllocatePort allocatePort;
  NewPageTestWritePort writePort;
  NewPageTestEvictionPolicy evictionPolicy;
  NewPageAction action(bufferPool, allocatePort, writePort, evictionPolicy);

  EXPECT_EQ(action.execute(), 2);
  EXPECT_EQ(writePort.writeCount, 0);
  EXPECT_EQ(frame.getPageId(), 2);
  EXPECT_EQ(frame.getPinCount(), 1);
  EXPECT_TRUE(frame.isDirty());
  EXPECT_EQ(frame.getPage().data()[0], 0);
}

// Scenario 3: If allocation of a replacement page fails, the evicted page
// remains resident and can be selected on retry.
TEST(NewPageActionTest, AllocationFailureRestoresVictimForRetry) {
  BufferPool bufferPool(1);
  Frame &frame = bufferPool.getFrame(0);
  frame.setPageId(1);
  frame.setOccupied(true);
  frame.setDirty(false);
  NewPageTestAllocatePort allocatePort;
  NewPageTestWritePort writePort;
  FIFOEvictionPolicy evictionPolicy;
  evictionPolicy.RecordAccess(1);
  evictionPolicy.SetEvictable(1, true);
  allocatePort.shouldThrow = true;
  NewPageAction action(bufferPool, allocatePort, writePort, evictionPolicy);

  EXPECT_THROW(action.execute(), std::runtime_error);
  EXPECT_EQ(frame.getPageId(), 1);
  EXPECT_TRUE(frame.isOccupied());
  EXPECT_EQ(frame.getPinCount(), 0);

  allocatePort.shouldThrow = false;
  EXPECT_EQ(action.execute(), 2);
  EXPECT_EQ(frame.getPageId(), 2);
  EXPECT_EQ(frame.getPinCount(), 1);
}

// Scenario 4: If dirty-victim writeback fails, the original page stays dirty
// and remains evictable for a retry.
TEST(NewPageActionTest, FailedVictimWriteRestoresVictimForRetry) {
  BufferPool bufferPool(1);
  Frame &frame = bufferPool.getFrame(0);
  BufferPoolPage oldPage;
  oldPage.data()[0] = 'D';
  frame.setPage(oldPage);
  frame.setPageId(1);
  frame.setOccupied(true);
  frame.setDirty(true);
  NewPageTestAllocatePort allocatePort;
  NewPageTestWritePort writePort;
  writePort.shouldThrow = true;
  FIFOEvictionPolicy evictionPolicy;
  evictionPolicy.RecordAccess(1);
  evictionPolicy.SetEvictable(1, true);
  NewPageAction action(bufferPool, allocatePort, writePort, evictionPolicy);

  EXPECT_THROW(action.execute(), std::runtime_error);
  EXPECT_EQ(frame.getPageId(), 1);
  EXPECT_TRUE(frame.isDirty());
  EXPECT_EQ(frame.getPinCount(), 0);

  writePort.shouldThrow = false;
  EXPECT_EQ(action.execute(), 2);
  EXPECT_EQ(frame.getPageId(), 2);
  EXPECT_EQ(writePort.writeCount, 2);
}

// Scenario 5: A policy registration failure on an empty frame rolls back the
// frame so a subsequent NewPage can safely reuse it.
TEST(NewPageActionTest, EmptyFramePolicyFailureRollsBackFrame) {
  BufferPool pool(1);
  NewPageTestAllocatePort allocatePort;
  NewPageTestWritePort writePort;
  NewPageTestEvictionPolicy policy;
  policy.throwNextRecordAccess = true;
  NewPageAction action(pool, allocatePort, writePort, policy);

  EXPECT_THROW(action.execute(), std::runtime_error);
  EXPECT_FALSE(pool.getFrame(0).isOccupied());
  EXPECT_EQ(pool.getFrame(0).getPinCount(), 0);

  EXPECT_NO_THROW(action.execute());
  EXPECT_TRUE(pool.getFrame(0).isOccupied());
  EXPECT_EQ(pool.getFrame(0).getPinCount(), 1);
}

// Scenario 6: A policy failure after victim replacement restores the original
// page and makes it selectable again for retry.
TEST(NewPageActionTest, ReplacementPolicyFailureRestoresVictimForRetry) {
  BufferPool pool(1);
  Frame &frame = pool.getFrame(0);
  BufferPoolPage oldPage;
  oldPage.data()[0] = 'R';
  frame.setPage(oldPage);
  frame.setPageId(1);
  frame.setOccupied(true);
  frame.setDirty(false);

  NewPageTestAllocatePort allocatePort;
  NewPageTestWritePort writePort;
  NewPageTestEvictionPolicy policy;
  policy.victim = 1;
  policy.throwNextRecordAccess = true;
  NewPageAction action(pool, allocatePort, writePort, policy);

  EXPECT_THROW(action.execute(), std::runtime_error);
  EXPECT_TRUE(frame.isOccupied());
  EXPECT_EQ(frame.getPageId(), 1);
  EXPECT_EQ(frame.getPinCount(), 0);
  EXPECT_EQ(frame.getPage().data()[0], 'R');

  policy.victim = 1;
  EXPECT_NO_THROW(action.execute());
  EXPECT_EQ(frame.getPageId(), 3);
  EXPECT_EQ(frame.getPinCount(), 1);
}

// Scenario 7: SetEvictable failure on an empty frame also rolls back the
// partially registered page and leaves the frame reusable.
TEST(NewPageActionTest, EmptyFrameSetEvictableFailureRollsBackFrame) {
  BufferPool pool(1);
  NewPageTestAllocatePort allocatePort;
  NewPageTestWritePort writePort;
  NewPageTestEvictionPolicy policy;
  policy.throwNextSetEvictable = true;
  NewPageAction action(pool, allocatePort, writePort, policy);

  EXPECT_THROW(action.execute(), std::runtime_error);
  EXPECT_FALSE(pool.getFrame(0).isOccupied());
  EXPECT_EQ(pool.getFrame(0).getPinCount(), 0);

  EXPECT_NO_THROW(action.execute());
  EXPECT_TRUE(pool.getFrame(0).isOccupied());
  EXPECT_EQ(pool.getFrame(0).getPinCount(), 1);
}

// Scenario 8: SetEvictable failure after victim replacement restores the
// victim frame and allows a later allocation attempt.
TEST(NewPageActionTest, ReplacementSetEvictableFailureRestoresVictim) {
  BufferPool pool(1);
  Frame &frame = pool.getFrame(0);
  BufferPoolPage oldPage;
  oldPage.data()[0] = 'S';
  frame.setPage(oldPage);
  frame.setPageId(1);
  frame.setOccupied(true);
  frame.setDirty(false);

  NewPageTestAllocatePort allocatePort;
  NewPageTestWritePort writePort;
  NewPageTestEvictionPolicy policy;
  policy.victim = 1;
  policy.throwNextSetEvictable = true;
  NewPageAction action(pool, allocatePort, writePort, policy);

  EXPECT_THROW(action.execute(), std::runtime_error);
  EXPECT_TRUE(frame.isOccupied());
  EXPECT_EQ(frame.getPageId(), 1);
  EXPECT_EQ(frame.getPage().data()[0], 'S');
  EXPECT_EQ(frame.getPinCount(), 0);

  policy.victim = 1;
  EXPECT_NO_THROW(action.execute());
  EXPECT_EQ(frame.getPageId(), 3);
  EXPECT_EQ(frame.getPinCount(), 1);
}

// Scenario 9: A full pool with no evictable victim fails without changing the
// resident page or its pin count.
TEST(NewPageActionTest, FullPoolWithoutEvictableVictimFailsCleanly) {
  BufferPool pool(1);
  Frame &frame = pool.getFrame(0);
  frame.setPageId(1);
  frame.setOccupied(true);
  frame.pin();
  NewPageTestAllocatePort allocatePort;
  NewPageTestWritePort writePort;
  NewPageTestEvictionPolicy policy;
  policy.victim = std::nullopt;
  NewPageAction action(pool, allocatePort, writePort, policy);

  EXPECT_THROW(action.execute(), std::runtime_error);
  EXPECT_EQ(frame.getPageId(), 1);
  EXPECT_EQ(frame.getPinCount(), 1);
  EXPECT_TRUE(frame.isOccupied());
  EXPECT_EQ(allocatePort.nextPageId, 2);
}
