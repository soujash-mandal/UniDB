#include "../../../CatalogService/test/CatalogActionTestSupport.h"
#include "../../adapter/UpdateCatalogTableAdapter.h"

#include <gtest/gtest.h>

TEST(UpdateCatalogTableAdapterTest,
     PreservesExplicitSpaceMapPageIdWhenUpdatingCatalogTable) {
  InMemoryCatalogPageStore store;
  CatalogPage page;
  Table existing{};
  existing.tableId = 9;
  existing.name = "events";
  existing.firstFreeSpaceMapPageId = 91;
  existing.columns = {Column{"id", DataType::INT, 4, false}};
  page.insert(existing);
  store.seed(10, page);

  UpdateTableAction action(store, store, store, store);
  UpdateCatalogTableAdapter adapter(action);

  RdbTableDefinition updated;
  updated.tableId = 9;
  updated.name = "events";
  updated.columns = {RdbColumn{"id", RdbDataType::BIGINT, 8, false}};
  adapter.updateTable(10, updated, 91);

  const Table stored = store.pages.at(10).getByName("events");
  ASSERT_EQ(stored.columns.size(), 1U);
  EXPECT_EQ(stored.columns[0].type, DataType::BIGINT);
  EXPECT_EQ(stored.columns[0].size, 8U);
  EXPECT_EQ(stored.firstFreeSpaceMapPageId, 91U);
  EXPECT_FALSE(store.hasPinnedPages());
}
