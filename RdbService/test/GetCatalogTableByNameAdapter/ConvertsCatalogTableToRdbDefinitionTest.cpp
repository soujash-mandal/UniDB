#include "../../../CatalogService/test/CatalogActionTestSupport.h"
#include "../../adapter/GetCatalogTableByNameAdapter.h"

#include <gtest/gtest.h>

TEST(GetCatalogTableByNameAdapterTest,
     ConvertsCatalogTableIntoRdbOwnedDefinition) {
  InMemoryCatalogPageStore store;
  CatalogPage page;
  Table table{};
  table.tableId = 42;
  table.name = "items";
  table.firstFreeSpaceMapPageId = 91;
  table.columns = {
      Column{"id", DataType::BIGINT, 8, false},
      Column{"name", DataType::VARCHAR, 80, true},
      Column{"active", DataType::BOOLEAN, 1, false},
  };
  page.insert(table);
  store.seed(10, page);

  GetTableByNameAction action(store, store);
  GetCatalogTableByNameAdapter adapter(action);

  const RdbTableDefinition result = adapter.getTableByName(10, "items");

  EXPECT_EQ(result.tableId, 42U);
  EXPECT_EQ(result.name, "items");
  EXPECT_EQ(result.firstSpaceMapPageId, 91U);
  ASSERT_EQ(result.columns.size(), 3U);
  EXPECT_EQ(result.columns[0].type, RdbDataType::BIGINT);
  EXPECT_EQ(result.columns[1].type, RdbDataType::VARCHAR);
  EXPECT_EQ(result.columns[1].size, 80U);
  EXPECT_TRUE(result.columns[1].nullable);
  EXPECT_EQ(result.columns[2].type, RdbDataType::BOOLEAN);
  EXPECT_FALSE(store.hasPinnedPages());
}
