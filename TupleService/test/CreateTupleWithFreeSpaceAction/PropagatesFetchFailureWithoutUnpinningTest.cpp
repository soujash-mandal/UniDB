#include "../../actions/CreateTupleWithFreeSpaceAction.h"
#include "../TupleActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>

// Scenario 1: Do not unpin when fetching fails before a page was acquired.
TEST(CreateTupleWithFreeSpaceActionTest, PropagatesFetchFailureWithoutUnpinning) {
  InMemoryTuplePageStore store;
  store.failFetch = true;
  CreateTupleWithFreeSpaceAction action(store, store, store);

  EXPECT_THROW(action.execute(7, {'x'}), std::runtime_error);
  EXPECT_EQ(store.unpinCalls, 0);
}
