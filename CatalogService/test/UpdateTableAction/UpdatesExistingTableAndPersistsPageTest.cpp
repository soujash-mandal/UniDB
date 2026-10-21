#include "../../actions/UpdateTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(UpdateTableActionTest, UpdatesExistingTableAndPersistsPage) {
InMemoryCatalogPageStore store;
  CatalogPage page;
  Table table{};
  table.tableId = 9;
  table.name = "events";
  page.insert(table);
  store.seed(10, page);
  UpdateTableAction action(store, store, store);
  table.firstFreeSpaceMapPageId = 123;

  action.execute(10, table);

  EXPECT_EQ(store.pages.at(10).get(9).firstFreeSpaceMapPageId, 123U);
  EXPECT_EQ(store.writtenPageIds, (std::vector<CatalogPageId>{10}));
  EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{10, 10}));
  EXPECT_FALSE(store.hasPinnedPages());
}
