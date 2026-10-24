#include "../../../SpaceMapService/actions/CreateSpaceMapPageAction.h"
#include "../../../SpaceMapService/actions/FindPageWithSpaceAction.h"
#include "../../adapter/FindPageWithSpaceAdapter.h"
#include "../StorageAdapterTestSupport.h"
#include <gtest/gtest.h>
TEST(FindPageWithSpaceAdapterTest, DelegatesPageDiscovery) {
  FakeSpaceMapStore store;
  CreateSpaceMapPageAction create(store, store, store);
  const auto root = create.execute();
  FindPageWithSpaceAction action(store, store, store, store, store);
  FindPageWithSpaceAdapter adapter(action);
  const auto result = adapter.findPageWithSpace(root, 100U);
  EXPECT_EQ(result.tuplePageId, 200U);
  EXPECT_EQ(result.spaceMapPageId, root);
}
