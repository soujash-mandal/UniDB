#include "../../actions/CreateTupleAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>

#include <stdexcept>

// Scenario 1: Propagate page fetch failures without leaking pins.
TEST(CreateTupleActionTest, PropagatesFetchFailure) {
  InMemoryTuplePageStore store;
  store.failFetch = true;
  CreateTupleAction action(store, store, store);

  EXPECT_THROW(action.execute(7, {'x'}), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
