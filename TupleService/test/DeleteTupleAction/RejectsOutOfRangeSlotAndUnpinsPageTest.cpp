#include "../../actions/DeleteTupleAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>

#include <stdexcept>

// Scenario 1: Reject out-of-range tuple slots and release the page pin.
TEST(DeleteTupleActionTest, RejectsOutOfRangeSlotAndUnpinsPage) {
  InMemoryTuplePageStore store;
  store.seed(9, TuplePage{});
  DeleteTupleAction action(store, store, store);

  EXPECT_THROW(action.execute(9, 0), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
