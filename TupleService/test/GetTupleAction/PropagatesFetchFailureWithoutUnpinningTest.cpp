#include "../../actions/GetTupleAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>

#include <stdexcept>

// Scenario 1: Propagate fetch failures; no page pin is acquired on failure.
TEST(GetTupleActionTest, PropagatesFetchFailureWithoutUnpinning) {
  InMemoryTuplePageStore store;
  store.failFetch = true;
  GetTupleAction action(store, store);

  EXPECT_THROW(action.execute(11, 0), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
