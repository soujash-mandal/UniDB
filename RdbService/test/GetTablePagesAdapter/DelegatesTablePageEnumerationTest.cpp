#include "../../../SpaceMapService/actions/CreateSpaceMapPageAction.h"
#include "../../../SpaceMapService/actions/GetTablePagesAction.h"
#include "../../adapter/GetTablePagesAdapter.h"
#include "../StorageAdapterTestSupport.h"
#include <gtest/gtest.h>
TEST(GetTablePagesAdapterTest, DelegatesTablePageEnumeration) {
  FakeSpaceMapStore store;
  CreateSpaceMapPageAction create(store, store, store);
  const auto root = create.execute();
  auto page = store.pages.at(root);
  page.insert(200, 7000);
  page.insert(201, 6000);
  store.pages[root] = page;
  GetTablePagesAction action(store, store);
  GetTablePagesAdapter adapter(action);
  EXPECT_EQ(adapter.getTablePages(root), (std::vector<uint32_t>{200U, 201U}));
}
