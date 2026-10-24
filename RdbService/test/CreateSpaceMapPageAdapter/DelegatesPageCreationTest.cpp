#include "../../adapter/CreateSpaceMapPageAdapter.h"
#include "../StorageAdapterTestSupport.h"
#include "../../../SpaceMapService/actions/CreateSpaceMapPageAction.h"
#include <gtest/gtest.h>
TEST(CreateSpaceMapPageAdapterTest, DelegatesPageCreation) {
  FakeSpaceMapStore store;
  CreateSpaceMapPageAction action(store, store, store);
  CreateSpaceMapPageAdapter adapter(action);
  EXPECT_EQ(adapter.createSpaceMapPage(), 20U);
  EXPECT_EQ(store.pages.size(), 1U);
}
