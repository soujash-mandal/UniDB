#include "../../actions/CreateTupleWithFreeSpaceAction.h"
#include "../TupleActionTestSupport.h"
#include <gtest/gtest.h>

// Scenario 1: Return the inserted slot and exact remaining space after the slot entry is accounted for.
TEST(CreateTupleWithFreeSpaceActionTest, ReturnsSlotAndActualRemainingFreeSpace) {
  InMemoryTuplePageStore store;
  store.seed(7, TuplePage{});
  CreateTupleWithFreeSpaceAction action(store, store, store);

  const auto result = action.execute(7, {'a', 'b', 'c'});

  EXPECT_EQ(result.slotId, 0U);
  EXPECT_EQ(result.remainingFreeSpace, store.pages.at(7).getFreeSpace());
  EXPECT_EQ(result.remainingFreeSpace, 8172U);
  EXPECT_FALSE(store.hasPinnedPages());
}
