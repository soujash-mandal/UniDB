#include "../../actions/CreateTupleAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>

#include <stdexcept>

// Scenario 1: Release the page pin when persisting an inserted tuple fails.
TEST(CreateTupleActionTest, UnpinsWhenPageWriteFails) {
  InMemoryTuplePageStore store;
  store.seed(7, TuplePage{});
  store.failWrite = true;
  CreateTupleAction action(store, store, store);

  EXPECT_THROW(action.execute(7, {'x'}), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
