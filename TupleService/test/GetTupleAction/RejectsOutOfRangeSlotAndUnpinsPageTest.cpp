#include "../../actions/GetTupleAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>

#include <stdexcept>

// Scenario 1: Reject out-of-range tuple slots and unpin the page.
TEST(GetTupleActionTest, RejectsOutOfRangeSlotAndUnpinsPage) {
  InMemoryTuplePageStore store;
  store.seed(11, TuplePage{});
  GetTupleAction action(store, store);

  EXPECT_THROW(action.execute(11, 0), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
