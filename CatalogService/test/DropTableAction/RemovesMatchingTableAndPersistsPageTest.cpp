#include "../../actions/DropTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(DropTableActionTest, RemovesMatchingTableAndPersistsPage) {
  InMemoryCatalogPageStore store;
    CatalogPage page;
    Table table{};
    table.tableId = 3;
    table.name = "users";
    page.insert(table);
    store.seed(10, page);
    DropTableAction action(store, store, store);
    action.execute(10, 3);
    EXPECT_FALSE(store.pages.at(10).containsTableId(3));
    EXPECT_EQ(store.writtenPageIds, (std::vector<CatalogPageId>{10}));
    EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{10}));
    EXPECT_FALSE(store.hasPinnedPages());
}
