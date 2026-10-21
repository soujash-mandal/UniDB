#include "../../actions/UpdateTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(UpdateTableActionTest, UnpinsPageWhenWriteFails) {
  InMemoryCatalogPageStore store;
    CatalogPage page;
    Table table{};
    table.tableId = 9;
    table.name = "events";
    page.insert(table);
    store.seed(10, page);
    store.failNextWrite = true;
    UpdateTableAction action(store, store, store);
  
    EXPECT_THROW(action.execute(10, table), std::runtime_error);
    EXPECT_FALSE(store.hasPinnedPages());
}
