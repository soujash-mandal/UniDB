#include "GetTablePagesActionTestSupport.h"

// Scenario 1: Return every tuple page ID from a single SpaceMap page.
TEST(GetTablePagesActionTest, ReturnsTuplePagesFromSingleSpaceMapPage) {
  InMemorySpaceMapStore store;
  SpaceMapPage page;
  page.insert(100, 7000);
  page.insert(101, 6000);
  store.seed(10, page);
  GetTablePagesAction action(store, store);

  EXPECT_EQ(action.execute(10), (std::vector<TuplePageId>{100, 101}));
  EXPECT_EQ(store.unpinnedPages, (std::vector<SpaceMapPageId>{10}));
  EXPECT_FALSE(store.hasPinnedPages());
}
