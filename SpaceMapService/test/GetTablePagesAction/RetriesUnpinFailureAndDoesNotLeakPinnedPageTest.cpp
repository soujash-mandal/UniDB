#include "GetTablePagesActionTestSupport.h"

// Scenario 7: Retry cleanup when unpin reports failure before releasing the pin.
// This regression test should fail until GetTablePagesAction keeps the pin marked
// as owned until unpinPage succeeds.
TEST(GetTablePagesActionTest,
     RetriesUnpinFailureAndDoesNotLeakPinnedPage) {
  InMemorySpaceMapStore store;
  store.seed(10, SpaceMapPage{});
  store.failNextUnpinBeforeRelease = true;
  GetTablePagesAction action(store, store);

  EXPECT_THROW(action.execute(10), std::runtime_error);

  EXPECT_EQ(store.unpinCalls, 2);
  EXPECT_FALSE(store.hasPinnedPages());
  EXPECT_EQ(store.unpinnedPages, (std::vector<SpaceMapPageId>{10}));
}
