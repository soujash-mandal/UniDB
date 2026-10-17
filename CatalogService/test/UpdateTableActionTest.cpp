#include "../actions/UpdateTableAction.h"
#include "CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(UpdateTableActionTest, UpdatesExistingTableAndPersistsPage) {
  InMemoryCatalogPageStore store;
  CatalogPage page;
  Table table{};
  table.tableId = 9;
  table.name = "events";
  page.insert(table);
  store.seed(10, page);
  UpdateTableAction action(store, store, store);
  table.firstTuplePageId = 123;

  action.execute(10, table);

  EXPECT_EQ(store.pages.at(10).get(9).firstTuplePageId, 123U);
  EXPECT_EQ(store.writtenPageIds, (std::vector<CatalogPageId>{10}));
  EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{10}));
}

TEST(UpdateTableActionTest, ThrowsWhenTableDoesNotExist) {
  InMemoryCatalogPageStore store;
  store.seed(10, CatalogPage{});
  UpdateTableAction action(store, store, store);
  Table missing{};
  missing.tableId = 99;
  missing.name = "missing";

  EXPECT_THROW(action.execute(10, missing), std::runtime_error);
  EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{10}));
}
