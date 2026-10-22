#include "../../actions/CreateTuplePageAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>

#include <stdexcept>

// Scenario 1: Release the allocated page if initialization write fails.
TEST(CreateTuplePageActionTest, UnpinsAllocatedPageWhenInitializationWriteFails) {
  InMemoryTuplePageStore store;
  store.failWrite = true;
  CreateTuplePageAction action(store, store, store);

  EXPECT_THROW(action.execute(), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
