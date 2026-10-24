#include "GetTablePagesActionTestSupport.h"

// Scenario 4: A failed fetch must not unpin a page that was never fetched.
TEST(GetTablePagesActionTest,
     PropagatesFetchFailureWithoutUnpinningMissingPage) {
  InMemorySpaceMapStore store;
  GetTablePagesAction action(store, store);

  EXPECT_THROW(action.execute(10), std::runtime_error);
  EXPECT_TRUE(store.unpinnedPages.empty());
  EXPECT_FALSE(store.hasPinnedPages());
}
