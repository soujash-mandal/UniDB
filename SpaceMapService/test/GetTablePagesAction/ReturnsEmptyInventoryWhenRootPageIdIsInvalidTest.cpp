#include "GetTablePagesActionTestSupport.h"

// Scenario 5: An invalid SpaceMap root means the table has no tuple pages.
TEST(GetTablePagesActionTest, ReturnsEmptyInventoryWhenRootPageIdIsInvalid) {
  InMemorySpaceMapStore store;
  GetTablePagesAction action(store, store);

  EXPECT_TRUE(action.execute(UINT32_MAX).empty());
  EXPECT_TRUE(store.unpinnedPages.empty());
  EXPECT_FALSE(store.hasPinnedPages());
}
