#include "../../../CatalogService/test/CatalogActionTestSupport.h"
#include "../../adapter/CreateCatalogTableAdapter.h"

#include <gtest/gtest.h>

TEST(CreateCatalogTableAdapterTest,
     ConvertsRdbOwnedColumnsWhenCreatingCatalogTable) {
  InMemoryCatalogPageStore store;
  store.seed(10, CatalogPage{});

  CreateTableAction action(store, store, store, store);
  CreateCatalogTableAdapter adapter(action);

  RdbTableDefinition table;
  table.name = "events";
  table.columns = {
      RdbColumn{"event_id", RdbDataType::INT, 4, false},
      RdbColumn{"description", RdbDataType::VARCHAR, 120, true},
  };

  const uint32_t tableId = adapter.createTable(10, 7, table);
  const Table stored = store.pages.at(10).getByName("events");

  EXPECT_EQ(tableId, 7U);
  EXPECT_EQ(stored.tableId, 7U);
  ASSERT_EQ(stored.columns.size(), 2U);
  EXPECT_EQ(stored.columns[0].type, DataType::INT);
  EXPECT_EQ(stored.columns[1].type, DataType::VARCHAR);
  EXPECT_EQ(stored.columns[1].size, 120U);
  EXPECT_TRUE(stored.columns[1].nullable);
  EXPECT_FALSE(store.hasPinnedPages());
}
