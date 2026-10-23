#include "../../actions/CreateTuplePageAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>

#include <stdexcept>

// Scenario 1: Propagate page allocation failures.
TEST(CreateTuplePageActionTest, PropagatesAllocationFailure) {
  InMemoryTuplePageStore store;
  store.failAllocation = true;
  CreateTuplePageAction action(store, store, store);

  EXPECT_THROW(action.execute(), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
