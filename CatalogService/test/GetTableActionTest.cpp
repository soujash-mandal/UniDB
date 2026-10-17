#include "../actions/GetTableAction.h"
#include "CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(GetTableActionTest, ReturnsTableByIdAndUnpinsPage) {
  InMemoryCatalogPageStore store;
  CatalogPage page;
  Table table{};
  table.tableId = 5;
  table.name = "orders";
  page.insert(table);
  store.seed(10, page);
  GetTableAction action(store, store);

  const Table result = action.execute(10, 5);

  EXPECT_EQ(result.name, "orders");
  EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{10}));
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(GetTableActionTest, ThrowsWhenTableIdIsMissing) {
  InMemoryCatalogPageStore store;
  store.seed(10, CatalogPage{});
  GetTableAction action(store, store);

  EXPECT_THROW(action.execute(10, 404), std::runtime_error);
  EXPECT_EQ(store.unpinnedPageIds, (std::vector<CatalogPageId>{10}));
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(GetTableActionTest, FindsTableOnLaterCatalogPage) {
  InMemoryCatalogPageStore store;
  CatalogPage firstPage;
  firstPage.setNextPageId(11);
  CatalogPage secondPage;
  Table table{};
  table.tableId = 5;
  table.name = "orders";
  secondPage.insert(table);
  store.seed(10, firstPage);
  store.seed(11, secondPage);
  GetTableAction action(store, store);

  EXPECT_EQ(action.execute(10, 5).name, "orders");
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(GetTableActionTest, PropagatesFetchFailureWithoutPinning) {
  InMemoryCatalogPageStore store;
  store.failNextFetch = true;
  GetTableAction action(store, store);

  EXPECT_THROW(action.execute(10, 5), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(GetTableActionTest, RejectsCyclicCatalogChainWithoutLeakingPins) {
  InMemoryCatalogPageStore store;
  CatalogPage page;
  page.setNextPageId(10);
  store.seed(10, page);
  GetTableAction action(store, store);

  EXPECT_THROW(action.execute(10, 5), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}
