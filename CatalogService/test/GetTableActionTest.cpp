#include "../actions/GetTableAction.h"
#include "CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(GetTableActionTest, ReturnsTableByIdAndUnpinsPage) {
  InMemoryCatalogPageStore store;
  CatalogPage page;
  Table table{};
  table.tableId = 5;
  table.name = "orders";
  page.insert(table);
  store.seed(10, page);
  GetTableAction action(store, store);

  const Table result = action.execute(10, 5);

  EXPECT_EQ(result.name, "orders");
  EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{10}));
}

TEST(GetTableActionTest, ThrowsWhenTableIdIsMissing) {
  InMemoryCatalogPageStore store;
  store.seed(10, CatalogPage{});
  GetTableAction action(store, store);

  EXPECT_THROW(action.execute(10, 404), std::runtime_error);
  EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{10}));
}
