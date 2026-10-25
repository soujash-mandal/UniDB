#include "../../actions/CreateTupleWithFreeSpaceAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>
#include <stdexcept>

// Scenario 1: Release the fetched page pin when persisting the tuple fails.
TEST(CreateTupleWithFreeSpaceActionTest, UnpinsWhenPageWriteFails) {
  InMemoryTuplePageStore store;
  store.seed(7, TuplePage{});
  store.failWrite = true;
  CreateTupleWithFreeSpaceAction action(store, store, store);

  EXPECT_THROW(action.execute(7, {'x'}), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
