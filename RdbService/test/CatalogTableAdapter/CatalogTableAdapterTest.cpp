#include "../../../CatalogService/test/CatalogActionTestSupport.h"
#include "../../adapter/CatalogTablePortFacade.h"

#include <gtest/gtest.h>

TEST(CatalogTableAdapterTest, ConvertsCatalogTableIntoRdbOwnedDefinition) {
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

  CreateTableAction createAction(store, store, store, store);
  GetTableByNameAction getAction(store, store);
  UpdateTableAction updateAction(store, store, store, store);
  CreateCatalogTableAdapter createAdapter(createAction);
  GetCatalogTableByNameAdapter getAdapter(getAction);
  UpdateCatalogTableAdapter updateAdapter(updateAction);
  CatalogTablePortFacade port(createAdapter, getAdapter, updateAdapter);

  const RdbTableDefinition result = port.getTableByName(10, "items");

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

TEST(CatalogTableAdapterTest, ConvertsRdbOwnedColumnsWhenCreatingCatalogTable) {
  InMemoryCatalogPageStore store;
  store.seed(10, CatalogPage{});

  CreateTableAction createAction(store, store, store, store);
  GetTableByNameAction getAction(store, store);
  UpdateTableAction updateAction(store, store, store, store);
  CreateCatalogTableAdapter createAdapter(createAction);
  GetCatalogTableByNameAdapter getAdapter(getAction);
  UpdateCatalogTableAdapter updateAdapter(updateAction);
  CatalogTablePortFacade port(createAdapter, getAdapter, updateAdapter);

  RdbTableDefinition table;
  table.name = "events";
  table.columns = {
      RdbColumn{"event_id", RdbDataType::INT, 4, false},
      RdbColumn{"description", RdbDataType::VARCHAR, 120, true},
  };

  const uint32_t tableId = port.createTable(10, 7, table);
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

TEST(CatalogTableAdapterTest, UpdatesCatalogTableThroughSingleActionAdapter) {
  InMemoryCatalogPageStore store;
  CatalogPage page;
  Table existing{};
  existing.tableId = 9;
  existing.name = "events";
  existing.columns = {Column{"id", DataType::INT, 4, false}};
  page.insert(existing);
  store.seed(10, page);

  CreateTableAction createAction(store, store, store, store);
  GetTableByNameAction getAction(store, store);
  UpdateTableAction updateAction(store, store, store, store);
  CreateCatalogTableAdapter createAdapter(createAction);
  GetCatalogTableByNameAdapter getAdapter(getAction);
  UpdateCatalogTableAdapter updateAdapter(updateAction);
  CatalogTablePortFacade port(createAdapter, getAdapter, updateAdapter);

  RdbTableDefinition updated;
  updated.tableId = 9;
  updated.name = "events";
  updated.columns = {RdbColumn{"id", RdbDataType::BIGINT, 8, false}};
  port.updateTable(10, updated);

  const Table stored = store.pages.at(10).getByName("events");
  ASSERT_EQ(stored.columns.size(), 1U);
  EXPECT_EQ(stored.columns[0].type, DataType::BIGINT);
  EXPECT_EQ(stored.columns[0].size, 8U);
  EXPECT_FALSE(store.hasPinnedPages());
}
