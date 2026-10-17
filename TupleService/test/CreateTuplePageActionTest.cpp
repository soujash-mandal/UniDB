#include "../actions/CreateTuplePageAction.h"
#include "TupleActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(CreateTuplePageActionTest, AllocatesInitializesAndUnpinsPage) {
  InMemoryTuplePageStore store;
  CreateTuplePageAction action(store, store, store);

  const auto result = action.execute();

  EXPECT_EQ(result.pageId, 1U);
  EXPECT_GT(result.freeSpace, 0U);
  EXPECT_TRUE(store.pages.contains(result.pageId));
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(CreateTuplePageActionTest, UnpinsAllocatedPageWhenInitializationWriteFails) {
  InMemoryTuplePageStore store;
  store.failWrite = true;
  CreateTuplePageAction action(store, store, store);

  EXPECT_THROW(action.execute(), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(CreateTuplePageActionTest, PropagatesAllocationFailure) {
  InMemoryTuplePageStore store;
  store.failAllocation = true;
  CreateTuplePageAction action(store, store, store);

  EXPECT_THROW(action.execute(), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
