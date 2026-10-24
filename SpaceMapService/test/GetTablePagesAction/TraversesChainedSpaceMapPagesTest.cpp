#include "GetTablePagesActionTestSupport.h"

// Scenario 2: Traverse a chained SpaceMap list and preserve page order.
TEST(GetTablePagesActionTest, TraversesChainedSpaceMapPages) {
  InMemorySpaceMapStore store;
  SpaceMapPage first;
  first.insert(100, 7000);
  first.setNextPageId(11);
  SpaceMapPage second;
  second.insert(200, 5000);
  second.insert(201, 4000);
  store.seed(10, first);
  store.seed(11, second);
  GetTablePagesAction action(store, store);

  EXPECT_EQ(action.execute(10), (std::vector<TuplePageId>{100, 200, 201}));
  EXPECT_EQ(store.unpinnedPages, (std::vector<SpaceMapPageId>{10, 11}));
  EXPECT_FALSE(store.hasPinnedPages());
}
