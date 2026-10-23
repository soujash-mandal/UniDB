#include "../../actions/CreateTuplePageAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>

// Scenario 1: Allocate, initialize, and unpin a new tuple page.
TEST(CreateTuplePageActionTest, AllocatesInitializesAndUnpinsPage) {
  InMemoryTuplePageStore store;
  CreateTuplePageAction action(store, store, store);

  const auto result = action.execute();

  EXPECT_EQ(result.pageId, 1U);
  EXPECT_GT(result.freeSpace, 0U);
  EXPECT_TRUE(store.pages.contains(result.pageId));
  EXPECT_FALSE(store.hasPinnedPages());
}
