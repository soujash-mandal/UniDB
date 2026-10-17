#include "../actions/CreateTableAction.h"
#include "../actions/GetTableAction.h"
#include "CatalogActionTestSupport.h"

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

TEST(CreateTableActionTest, RejectsEmptyNameAndDuplicateName) {
  InMemoryCatalogPageStore store;
  store.seed(10, CatalogPage{});
  CreateTableAction action(store, store, store, store);

  EXPECT_THROW(action.execute(10, 1, "", {}), std::invalid_argument);
  action.execute(10, 1, "users", {});
  EXPECT_THROW(action.execute(10, 2, "users", {}), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(CreateTableActionTest, UnpinsPageWhenTableWriteFails) {
  InMemoryCatalogPageStore store;
  store.seed(10, CatalogPage{});
  store.failNextWrite = true;
  CreateTableAction action(store, store, store, store);

  EXPECT_THROW(action.execute(10, 1, "users", {}), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(CreateTableActionTest, ReleasesTailPageWhenAllocationFails) {
  InMemoryCatalogPageStore store;
  CatalogPage fullPage;
  Table wideTable{};
  wideTable.tableId = 1;
  wideTable.name = "wide";
  for (uint32_t columnId = 0; columnId < 203; ++columnId) {
    wideTable.columns.push_back(
        {std::string(32, static_cast<char>('a' + columnId % 26)),
         DataType::INT, 0, false});
  }
  fullPage.insert(wideTable);
  store.seed(10, fullPage);
  store.failNextAllocation = true;
  CreateTableAction action(store, store, store, store);

  EXPECT_THROW(action.execute(10, 999, "a-new-table", {}), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(CreateTableActionTest, PropagatesFetchFailureWithoutLeakingPins) {
  InMemoryCatalogPageStore store;
  store.seed(10, CatalogPage{});
  store.failNextFetch = true;
  CreateTableAction action(store, store, store, store);

  EXPECT_THROW(action.execute(10, 1, "users", {}), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(CreateTableActionTest, RejectsCyclicCatalogChainWithoutLeakingPins) {
  InMemoryCatalogPageStore store;
  CatalogPage page;
  page.setNextPageId(10);
  store.seed(10, page);
  CreateTableAction action(store, store, store, store);

  EXPECT_THROW(action.execute(10, 2, "users", {}), std::runtime_error);
  EXPECT_FALSE(store.hasPinnedPages());
}

TEST(CreateTableActionTest, CreatesTableOnNewPageWhenTailHasNoSpace) {
  InMemoryCatalogPageStore store;
  CatalogPage fullPage;
  Table wideTable{};
  wideTable.tableId = 1;
  wideTable.name = "wide";
  for (uint32_t columnId = 0; columnId < 203; ++columnId) {
    wideTable.columns.push_back(
        {std::string(32, static_cast<char>('a' + columnId % 26)),
         DataType::INT, 0, false});
  }
  fullPage.insert(wideTable);
  store.seed(10, fullPage);
  CreateTableAction action(store, store, store, store);

  EXPECT_EQ(action.execute(10, 999, "users", {}), 999U);

  GetTableAction getTable(store, store);
  EXPECT_EQ(getTable.execute(10, 999).name, "users");
  EXPECT_EQ(store.pages.at(10).getNextPageId(), 11U);
  EXPECT_FALSE(store.hasPinnedPages());
}
