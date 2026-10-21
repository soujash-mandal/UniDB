#include "../../actions/GetTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(GetTableActionTest, FindsTableOnLaterCatalogPage) {
  InMemoryCatalogPageStore store;
    CatalogPage firstPage;
    firstPage.setNextPageId(11);
    CatalogPage secondPage;
    Table table{};
    table.tableId = 5;
    table.name = "orders";
    secondPage.insert(table);
    store.seed(10, firstPage);
    store.seed(11, secondPage);
    GetTableAction action(store, store);
    EXPECT_EQ(action.execute(10, 5).name, "orders");
    EXPECT_FALSE(store.hasPinnedPages());
}
