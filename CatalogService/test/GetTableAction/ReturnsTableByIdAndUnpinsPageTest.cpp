#include "../../actions/GetTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

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
  EXPECT_FALSE(store.hasPinnedPages());
}
