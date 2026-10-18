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
  table.firstFreeSpaceMapPageId = 123;

  action.execute(10, table);

  EXPECT_EQ(store.pages.at(10).get(9).firstFreeSpaceMapPageId, 123U);
  EXPECT_EQ(store.writtenPageIds, (std::vector<CatalogPageId>{10}));
  EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{10}));
  EXPECT_FALSE(store.hasPinnedPages());
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
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(UpdateTableActionTest, PropagatesFetchFailureWithoutPinning) {
  InMemoryCatalogPageStore store;
  store.failNextFetch = true;
  UpdateTableAction action(store, store, store);
  Table table{};
  table.tableId = 9;
  table.name = "events";

  EXPECT_THROW(action.execute(10, table), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

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

TEST(UpdateTableActionTest, RejectsCyclicCatalogChainWithoutLeakingPins) {
  InMemoryCatalogPageStore store;
  CatalogPage page;
  page.setNextPageId(10);
  store.seed(10, page);
  UpdateTableAction action(store, store, store);
  Table table{};
  table.tableId = 9;
  table.name = "events";

  EXPECT_THROW(action.execute(10, table), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(UpdateTableActionTest, RejectsEmptyTableName) {
  InMemoryCatalogPageStore store;
  UpdateTableAction action(store, store, store);
  Table table{};
  table.tableId = 9;

  EXPECT_THROW(action.execute(10, table), std::invalid_argument);
  EXPECT_TRUE(store.unpinnedPageIds.empty());
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(UpdateTableActionTest, RejectsNameUsedByDifferentTableOnAnotherPage) {
  InMemoryCatalogPageStore store;
  CatalogPage root;
  root.setNextPageId(11);
  Table target{};
  target.tableId = 2;
  target.name = "orders";
  root.insert(target);

  CatalogPage second;
  Table existing{};
  existing.tableId = 1;
  existing.name = "users";
  second.insert(existing);

  store.seed(10, root);
  store.seed(11, second);
  UpdateTableAction action(store, store, store);
  target.name = "users";

  EXPECT_THROW(action.execute(10, target), std::runtime_error);
  EXPECT_TRUE(store.writtenPageIds.empty());
  EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{10, 11}));
  EXPECT_FALSE(store.hasPinnedPages());
}
