#include "../../actions/CreateTupleWithFreeSpaceAction.h"
#include "../TupleActionTestSupport.h"

#include <gtest/gtest.h>

// Scenario 1: Return the slot and actual remaining free space from the inserted
// page.
TEST(CreateTupleWithFreeSpaceActionTest, ReturnsSlotAndActualRemainingSpace) {
  InMemoryTuplePageStore store;
  store.seed(7, TuplePage{});
  CreateTupleWithFreeSpaceAction action(store, store, store);

  const auto result = action.execute(7, {'x', 'y'});

  EXPECT_EQ(result.slotId, 0);
  EXPECT_EQ(result.remainingFreeSpace, store.pages.at(7).getFreeSpace());
  EXPECT_FALSE(store.hasPinnedPages());
}
