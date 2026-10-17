#include "../actions/DeleteTupleAction.h"
#include "TupleActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>

TEST(DeleteTupleActionTest, DeletesTupleAndPersistsPage) {
  InMemoryTuplePageStore store;
  TuplePage page;
  page.insert({'x'});
  store.seed(9, page);
  DeleteTupleAction action(store, store, store);

  action.execute(9, 0);

  EXPECT_THROW(store.pages.at(9).get(0), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(DeleteTupleActionTest, RejectsOutOfRangeSlotAndUnpinsPage) {
  InMemoryTuplePageStore store;
  store.seed(9, TuplePage{});
  DeleteTupleAction action(store, store, store);

  EXPECT_THROW(action.execute(9, 0), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(DeleteTupleActionTest, UnpinsWhenPageWriteFails) {
  InMemoryTuplePageStore store;
  TuplePage page;
  page.insert({'x'});
  store.seed(9, page);
  store.failWrite = true;
  DeleteTupleAction action(store, store, store);

  EXPECT_THROW(action.execute(9, 0), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(DeleteTupleActionTest, PropagatesFetchFailure) {
  InMemoryTuplePageStore store;
  store.failFetch = true;
  DeleteTupleAction action(store, store, store);

  EXPECT_THROW(action.execute(9, 0), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
