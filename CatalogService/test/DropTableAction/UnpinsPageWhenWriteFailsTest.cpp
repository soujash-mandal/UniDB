#include "../../actions/DropTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(DropTableActionTest, UnpinsPageWhenWriteFails) {
InMemoryCatalogPageStore store;
  CatalogPage page;
  Table table{};
  table.tableId = 3;
  table.name = "users";
  page.insert(table);
  store.seed(10, page);
  store.failNextWrite = true;
  DropTableAction action(store, store, store);
  EXPECT_THROW(action.execute(10, 3), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
