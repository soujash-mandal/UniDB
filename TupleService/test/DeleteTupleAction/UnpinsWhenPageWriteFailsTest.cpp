#include "../../actions/DeleteTupleAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>

#include <stdexcept>

// Scenario 1: Release the page pin if persisting the deletion fails.
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
