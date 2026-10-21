#include "../../actions/GetTableByNameAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(GetTableByNameActionTest, FindsTableOnLaterCatalogPage) {
  InMemoryCatalogPageStore store;
    CatalogPage firstPage;
    firstPage.setNextPageId(11);
    CatalogPage secondPage;
    Table table{};
    table.tableId = 8;
    table.name = "products";
    secondPage.insert(table);
    store.seed(10, firstPage);
    store.seed(11, secondPage);
    GetTableByNameAction action(store, store);
  
    EXPECT_EQ(action.execute(10, "products").tableId, 8U);
    EXPECT_FALSE(store.hasPinnedPages());
}
