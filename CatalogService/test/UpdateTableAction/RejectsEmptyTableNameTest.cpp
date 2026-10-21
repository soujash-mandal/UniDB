#include "../../actions/UpdateTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(UpdateTableActionTest, RejectsEmptyTableName) {
  InMemoryCatalogPageStore store;
  UpdateTableAction action(store, store, store, store);
  Table table{};
  table.tableId = 9;

  EXPECT_THROW(action.execute(10, table), std::invalid_argument);
  EXPECT_TRUE(store.unpinnedPageIds.empty());
  EXPECT_FALSE(store.hasPinnedPages());
}
