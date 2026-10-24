#include "GetTablePagesActionTestSupport.h"

// Scenario 3: Reject cycles and release every page fetched before detection.
TEST(GetTablePagesActionTest, RejectsCycleWithoutLeakingPins) {
  InMemorySpaceMapStore store;
  SpaceMapPage first;
  first.setNextPageId(11);
  SpaceMapPage second;
  second.setNextPageId(10);
  store.seed(10, first);
  store.seed(11, second);
  GetTablePagesAction action(store, store);

  EXPECT_THROW(action.execute(10), std::runtime_error);
  EXPECT_EQ(store.unpinnedPages, (std::vector<SpaceMapPageId>{10, 11}));
  EXPECT_FALSE(store.hasPinnedPages());
}
