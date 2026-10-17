#include "../actions/CreateTupleAction.h"
#include "TupleActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>

TEST(CreateTupleActionTest, InsertsTupleWritesPageAndUnpins) {
  InMemoryTuplePageStore store;
  store.seed(7, TuplePage{});
  CreateTupleAction action(store, store, store);

  EXPECT_EQ(action.execute(7, {'a', 'b'}), 0U);
  EXPECT_EQ(store.pages.at(7).get(0), (std::vector<char>{'a', 'b'}));
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(CreateTupleActionTest, RejectsTupleThatCannotFitAndUnpins) {
  InMemoryTuplePageStore store;
  store.seed(7, TuplePage{});
  CreateTupleAction action(store, store, store);

  EXPECT_THROW(action.execute(7, std::vector<char>(8192, 'x')),
               std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(CreateTupleActionTest, UnpinsWhenPageWriteFails) {
  InMemoryTuplePageStore store;
  store.seed(7, TuplePage{});
  store.failWrite = true;
  CreateTupleAction action(store, store, store);

  EXPECT_THROW(action.execute(7, {'x'}), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(CreateTupleActionTest, PropagatesFetchFailure) {
  InMemoryTuplePageStore store;
  store.failFetch = true;
  CreateTupleAction action(store, store, store);

  EXPECT_THROW(action.execute(7, {'x'}), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
