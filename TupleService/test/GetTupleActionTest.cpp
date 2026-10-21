#include "../actions/GetTupleAction.h"
#include "TupleActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>

TEST(GetTupleActionTest, ReturnsTupleBytesAndUnpinsPage) {
  InMemoryTuplePageStore store;
  TuplePage page;
  page.insert({'o', 'k'});
  store.seed(11, page);
  GetTupleAction action(store, store);

  EXPECT_EQ(action.execute(11, 0), (std::vector<char>{'o', 'k'}));
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(GetTupleActionTest, RejectsOutOfRangeSlotAndUnpinsPage) {
  InMemoryTuplePageStore store;
  store.seed(11, TuplePage{});
  GetTupleAction action(store, store);

  EXPECT_THROW(action.execute(11, 0), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(GetTupleActionTest, RejectsDeletedSlotAndUnpinsPage) {
  InMemoryTuplePageStore store;
  TuplePage page;
  page.insert({'x'});
  page.remove(0);
  store.seed(11, page);
  GetTupleAction action(store, store);

  EXPECT_THROW(action.execute(11, 0), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(GetTupleActionTest, PropagatesFetchFailureWithoutUnpinning) {
  InMemoryTuplePageStore store;
  store.failFetch = true;
  GetTupleAction action(store, store);

  EXPECT_THROW(action.execute(11, 0), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
