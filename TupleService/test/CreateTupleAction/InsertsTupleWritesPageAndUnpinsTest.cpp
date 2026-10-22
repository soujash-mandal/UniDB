#include "../../actions/CreateTupleAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>

#include <vector>

// Scenario 1: Insert tuple bytes, persist the page, and unpin it.
TEST(CreateTupleActionTest, InsertsTupleWritesPageAndUnpins) {
  InMemoryTuplePageStore store;
  store.seed(7, TuplePage{});
  CreateTupleAction action(store, store, store);

  EXPECT_EQ(action.execute(7, {'a', 'b'}), 0U);
  EXPECT_EQ(store.pages.at(7).get(0), (std::vector<char>{'a', 'b'}));
  EXPECT_FALSE(store.hasPinnedPages());
}
