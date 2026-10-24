#include "FindPageWithSpaceActionTestSupport.h"

TEST(FindPageWithSpaceActionTest, ReturnsOwningSpaceMapPageFoundLaterInChain) {
  FindPageWithSpaceTestStore store;
  SpaceMapPage root;
  root.setNextPageId(11);
  SpaceMapPage second;
  second.insert(77, 512);
  store.seed(10, root);
  store.seed(11, second);

  FindPageWithSpaceAction action(store, store, store, store, store);
  const FindPageWithSpaceResult result = action.execute(10, 256);

  EXPECT_EQ(result.tuplePageId, 77);
  EXPECT_EQ(result.spaceMapPageId, 11);
  EXPECT_EQ(store.unpinnedPages, (std::vector<SpaceMapPageId>{10, 11}));
  EXPECT_TRUE(store.pinnedPages.empty());
  EXPECT_EQ(store.createTuplePageCalls, 0);
}
