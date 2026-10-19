#include <chrono>
#include <condition_variable>
#include <functional>
#include <gtest/gtest.h>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>

#include "../actions/NewPageAction.h"
#include "../actions/UnpinPageAction.h"

class UnpinTestEvictionPolicy : public EvictionPolicy {
public:
  int recordAccessCount = 0;
  int setEvictableCount = 0;
  BufferPoolPageId lastPageId = 0;
  bool lastEvictable = false;
  bool shouldThrowOnEvictable = false;
  bool shouldThrowAfterCallback = false;
  std::function<void()> onMakeEvictable;
  std::optional<BufferPoolPageId> victim;

  void RecordAccess(BufferPoolPageId) override { ++recordAccessCount; }
  void SetEvictable(BufferPoolPageId pageId, bool evictable) override {
    ++setEvictableCount;
    lastPageId = pageId;
    lastEvictable = evictable;
    if (shouldThrowOnEvictable) {
      shouldThrowOnEvictable = false;
      throw std::runtime_error("simulated policy failure");
    }
    if (evictable && onMakeEvictable) {
      onMakeEvictable();
    }
    if (shouldThrowAfterCallback) {
      shouldThrowAfterCallback = false;
      throw std::runtime_error("simulated post-callback policy failure");
    }
  }
  std::optional<BufferPoolPageId> Evict() override { return victim; }
  void Remove(BufferPoolPageId) override {}
};

namespace {
void OccupyFrame(BufferPool &pool, BufferPoolPageId pageId, uint32_t pinCount) {
  Frame &frame = pool.getFrame(0);
  frame.setPageId(pageId);
  frame.setOccupied(true);
  for (uint32_t i = 0; i < pinCount; ++i) {
    frame.pin();
  }
}
} // namespace

// Scenario 1: Unpinning the last pin releases the page for eviction.
TEST(UnpinPageActionTest, LastUnpinMakesPageEvictable) {
  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 1);
  UnpinPageAction action(pool, policy);

  EXPECT_NO_THROW(action.execute(7));

  EXPECT_EQ(pool.getFrame(0).getPinCount(), 0);
  EXPECT_EQ(policy.setEvictableCount, 1);
  EXPECT_EQ(policy.lastPageId, 7);
  EXPECT_TRUE(policy.lastEvictable);
}

// Scenario 2: A page with multiple pins stays non-evictable until its final
// unpin.
TEST(UnpinPageActionTest, PageRemainsNonEvictableWhileAnotherPinExists) {
  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 2);
  UnpinPageAction action(pool, policy);

  action.execute(7);

  EXPECT_EQ(pool.getFrame(0).getPinCount(), 1);
  EXPECT_EQ(policy.setEvictableCount, 0);

  action.execute(7);

  EXPECT_EQ(pool.getFrame(0).getPinCount(), 0);
  EXPECT_EQ(policy.setEvictableCount, 1);
  EXPECT_TRUE(policy.lastEvictable);
}

// Scenario 3: Unpinning a page that is not resident fails without changing
// another frame.
TEST(UnpinPageActionTest, UnknownPageThrowsWithoutChangingResidentPage) {
  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 1);
  UnpinPageAction action(pool, policy);

  EXPECT_THROW(action.execute(99), std::runtime_error);

  EXPECT_EQ(pool.getFrame(0).getPageId(), 7);
  EXPECT_EQ(pool.getFrame(0).getPinCount(), 1);
  EXPECT_EQ(policy.setEvictableCount, 0);
}

// Scenario 4: A second unpin after the pin count reaches zero is rejected and
// does not corrupt policy state.
TEST(UnpinPageActionTest, RejectsPinCountUnderflow) {
  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 1);
  UnpinPageAction action(pool, policy);

  action.execute(7);
  ASSERT_EQ(pool.getFrame(0).getPinCount(), 0);
  ASSERT_EQ(policy.setEvictableCount, 1);

  EXPECT_THROW(action.execute(7), std::runtime_error);

  EXPECT_EQ(pool.getFrame(0).getPinCount(), 0);
  EXPECT_EQ(policy.setEvictableCount, 1);
}

// Scenario 5: If making the page evictable fails, its final pin is retained
// so the caller can retry without policy/frame divergence.
TEST(UnpinPageActionTest, PolicyFailurePreservesFinalPinForRetry) {
  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 1);
  UnpinPageAction action(pool, policy);
  policy.shouldThrowOnEvictable = true;

  EXPECT_THROW(action.execute(7), std::runtime_error);
  EXPECT_EQ(pool.getFrame(0).getPinCount(), 1);

  EXPECT_NO_THROW(action.execute(7));
  EXPECT_EQ(pool.getFrame(0).getPinCount(), 0);
  EXPECT_TRUE(policy.lastEvictable);
}

class UnpinTestAllocatePort : public AllocatePagePort {
public:
  BufferPoolPageId allocatePage() override { return 8; }
};

class UnpinTestWritePort : public WritePagePort {
public:
  void writePage(BufferPoolPageId, BufferPoolPage) override {}
};

// Scenario 6: Reproduce replacement between SetEvictable(true) and the
// original unpin by invoking the real NewPageAction from the policy callback.
// The replacement page must start with exactly its own pin, not inherit the
// victim page's outstanding pin.
TEST(UnpinPageActionTest, ReplacementDoesNotInheritVictimPin) {
  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 1);
  UnpinPageAction unpinAction(pool, policy);
  UnpinTestAllocatePort allocatePort;
  UnpinTestWritePort writePort;
  NewPageAction newPageAction(pool, allocatePort, writePort, policy);
  policy.victim = 7;

  uint32_t replacementPinCountBeforeOldUnpin = 0;
  policy.onMakeEvictable = [&]() {
    EXPECT_EQ(newPageAction.execute(), 8);
    replacementPinCountBeforeOldUnpin = pool.getFrame(0).getPinCount();
  };

  unpinAction.execute(7);

  EXPECT_EQ(pool.getFrame(0).getPageId(), 8);
  EXPECT_EQ(replacementPinCountBeforeOldUnpin, 1);
  EXPECT_EQ(pool.getFrame(0).getPinCount(), 1);
}


// Scenario 7: A replacement must not run while the final-unpin operation
// holds the buffer-pool synchronization lock.
TEST(UnpinPageActionTest, ReplacementWaitsForFinalUnpinToFinish) {
  using namespace std::chrono_literals;

  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 1);
  UnpinPageAction unpinAction(pool, policy);
  UnpinTestAllocatePort allocatePort;
  UnpinTestWritePort writePort;
  NewPageAction newPageAction(pool, allocatePort, writePort, policy);
  policy.victim = 7;

  std::mutex callbackMutex;
  std::condition_variable callbackCondition;
  bool callbackEntered = false;
  bool releaseCallback = false;
  policy.onMakeEvictable = [&]() {
    std::unique_lock<std::mutex> lock(callbackMutex);
    callbackEntered = true;
    callbackCondition.notify_one();
    callbackCondition.wait(lock, [&]() { return releaseCallback; });
  };

  std::thread unpinThread([&]() { unpinAction.execute(7); });
  {
    std::unique_lock<std::mutex> lock(callbackMutex);
    ASSERT_TRUE(callbackCondition.wait_for(
        lock, 2s, [&]() { return callbackEntered; }));
  }

  std::mutex replacementMutex;
  std::condition_variable replacementCondition;
  bool replacementStarted = false;
  bool replacementCompleted = false;
  std::thread replacementThread([&]() {
    {
      std::lock_guard<std::mutex> lock(replacementMutex);
      replacementStarted = true;
      replacementCondition.notify_one();
    }
    EXPECT_EQ(newPageAction.execute(), 8);
    {
      std::lock_guard<std::mutex> lock(replacementMutex);
      replacementCompleted = true;
      replacementCondition.notify_one();
    }
  });

  bool completedWhileUnpinBlocked = false;
  {
    std::unique_lock<std::mutex> lock(replacementMutex);
    ASSERT_TRUE(replacementCondition.wait_for(
        lock, 2s, [&]() { return replacementStarted; }));
    completedWhileUnpinBlocked = replacementCondition.wait_for(
        lock, 200ms, [&]() { return replacementCompleted; });
  }

  {
    std::lock_guard<std::mutex> lock(callbackMutex);
    releaseCallback = true;
  }
  callbackCondition.notify_one();
  unpinThread.join();
  replacementThread.join();

  EXPECT_FALSE(completedWhileUnpinBlocked);
  EXPECT_EQ(pool.getFrame(0).getPageId(), 8);
  EXPECT_EQ(pool.getFrame(0).getPinCount(), 1);
}

// Scenario 8: If policy update throws after a replacement callback, rollback
// must not pin the replacement page.
TEST(UnpinPageActionTest, PolicyFailureAfterReplacementDoesNotPinReplacement) {
  BufferPool pool(1);
  UnpinTestEvictionPolicy policy;
  OccupyFrame(pool, 7, 1);
  UnpinPageAction unpinAction(pool, policy);
  UnpinTestAllocatePort allocatePort;
  UnpinTestWritePort writePort;
  NewPageAction newPageAction(pool, allocatePort, writePort, policy);
  policy.victim = 7;
  policy.shouldThrowAfterCallback = true;
  policy.onMakeEvictable = [&]() { EXPECT_EQ(newPageAction.execute(), 8); };

  EXPECT_THROW(unpinAction.execute(7), std::runtime_error);

  EXPECT_EQ(pool.getFrame(0).getPageId(), 8);
  EXPECT_EQ(pool.getFrame(0).getPinCount(), 1);
}
