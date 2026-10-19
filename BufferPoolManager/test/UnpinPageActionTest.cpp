#include "../actions/UnpinPageAction.h"

#include <gtest/gtest.h>

#include <optional>
#include <stdexcept>

class UnpinTestEvictionPolicy : public EvictionPolicy {
public:
  int recordAccessCount = 0;
  int setEvictableCount = 0;
  BufferPoolPageId lastPageId = 0;
  bool lastEvictable = false;
  bool shouldThrowOnEvictable = false;

  void RecordAccess(BufferPoolPageId) override { ++recordAccessCount; }
  void SetEvictable(BufferPoolPageId pageId, bool evictable) override {
    ++setEvictableCount;
    lastPageId = pageId;
    lastEvictable = evictable;
    if (shouldThrowOnEvictable) {
      shouldThrowOnEvictable = false;
      throw std::runtime_error("simulated policy failure");
    }
  }
  std::optional<BufferPoolPageId> Evict() override { return std::nullopt; }
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
