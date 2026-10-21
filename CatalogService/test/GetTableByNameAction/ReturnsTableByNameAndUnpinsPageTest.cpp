#include "../../actions/GetTableByNameAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(GetTableByNameActionTest, ReturnsTableByNameAndUnpinsPage) {
  InMemoryCatalogPageStore store;
  CatalogPage page;
  Table table{};
  table.tableId = 8;
  table.name = "products";
  page.insert(table);
  store.seed(10, page);
  GetTableByNameAction action(store, store);

  const Table result = action.execute(10, "products");

  EXPECT_EQ(result.tableId, 8U);
  EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{10}));
  EXPECT_FALSE(store.hasPinnedPages());
}
