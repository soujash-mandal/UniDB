#include "FindPageWithSpaceActionTestSupport.h"

TEST(FindPageWithSpaceActionTest,
     ReturnsOwningSpaceMapPageWhenItCreatesAndRegistersTuplePage) {
  FindPageWithSpaceTestStore store;
  store.seed(10, SpaceMapPage{});

  FindPageWithSpaceAction action(store, store, store, store, store);
  const FindPageWithSpaceResult result = action.execute(10, 128);

  EXPECT_EQ(result.tuplePageId, 300);
  EXPECT_EQ(result.spaceMapPageId, 10);
  EXPECT_EQ(store.createTuplePageCalls, 1);
  EXPECT_EQ(store.pages.at(10).findPageWithSpace(128), 300);
  EXPECT_EQ(store.unpinnedPages, (std::vector<SpaceMapPageId>{10}));
  EXPECT_TRUE(store.pinnedPages.empty());
}
