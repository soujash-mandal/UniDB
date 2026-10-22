#include "../../actions/CreateTupleAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>

// Scenario 1: Reject oversized tuples and release the fetched page.
TEST(CreateTupleActionTest, RejectsTupleThatCannotFitAndUnpins) {
  InMemoryTuplePageStore store;
  store.seed(7, TuplePage{});
  CreateTupleAction action(store, store, store);

  EXPECT_THROW(action.execute(7, std::vector<char>(8192, 'x')),
               std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
