#include "TestSupport.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
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
