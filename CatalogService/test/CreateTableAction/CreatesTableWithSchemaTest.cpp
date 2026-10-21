#include "../../actions/CreateTableAction.h"
#include "../../actions/GetTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

TEST(CreateTableActionTest, CreatesTableWithSchema) {
  InMemoryCatalogPageStore store;
  store.seed(10, CatalogPage{});
  CreateTableAction action(store, store, store, store);
  const std::vector<Column> columns = {{"id", DataType::INT, 0, false},
                                       {"name", DataType::VARCHAR, 64, true}};
  EXPECT_EQ(action.execute(10, 7, "users", columns), 7U);
  GetTableAction getTable(store, store);
  const Table table = getTable.execute(10, 7);
  EXPECT_EQ(table.name, "users");
  ASSERT_EQ(table.columns.size(), 2U);
  EXPECT_EQ(table.columns[1].name, "name");
  EXPECT_FALSE(store.hasPinnedPages());
}
