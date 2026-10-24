#include "FindPageWithSpaceActionTestSupport.h"

TEST(FindPageWithSpaceActionTest,
     ReturnsTuplePageAndOwningSpaceMapPageWhenSpaceIsAvailable) {
  FindPageWithSpaceTestStore store;
  SpaceMapPage root;
  root.insert(42, 256);
  store.seed(10, root);

  FindPageWithSpaceAction action(store, store, store, store, store);
  const FindPageWithSpaceResult result = action.execute(10, 128);

  EXPECT_EQ(result.tuplePageId, 42);
  EXPECT_EQ(result.spaceMapPageId, 10);
  EXPECT_EQ(store.unpinnedPages, (std::vector<SpaceMapPageId>{10}));
  EXPECT_TRUE(store.pinnedPages.empty());
  EXPECT_EQ(store.createTuplePageCalls, 0);
}
