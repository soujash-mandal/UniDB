#include "../../actions/DeleteTupleAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>

#include <stdexcept>

// Scenario 1: Delete a tuple and persist the updated page.
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
