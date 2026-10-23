#include "../../actions/DeleteTupleAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>

#include <stdexcept>

// Scenario 1: Propagate fetch failures without leaking page pins.
TEST(DeleteTupleActionTest, PropagatesFetchFailure) {
  InMemoryTuplePageStore store;
  store.failFetch = true;
  DeleteTupleAction action(store, store, store);

  EXPECT_THROW(action.execute(9, 0), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
