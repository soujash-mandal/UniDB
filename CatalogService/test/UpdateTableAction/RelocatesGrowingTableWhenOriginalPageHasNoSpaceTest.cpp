#include "../../actions/GetTableAction.h"
#include "../../actions/UpdateTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {
std::vector<Column> makeColumns(uint32_t count) {
  std::vector<Column> columns;
  columns.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    Column column{};
    column.name = std::string(32, static_cast<char>('a' + i % 26));
    column.type = DataType::INT;
    column.size = 0;
    column.nullable = false;
    columns.push_back(column);
  }
  return columns;
}
} // namespace

TEST(UpdateTableActionTest, RelocatesGrowingTableWhenOriginalPageHasNoSpace) {
  InMemoryCatalogPageStore store;
  CatalogPage root;
  Table filler{};
  filler.tableId = 1;
  filler.name = "filler";
  filler.columns = makeColumns(130);
  root.insert(filler);

  Table target{};
  target.tableId = 2;
  target.name = "target";
  target.columns = makeColumns(50);
  root.insert(target);
  target.columns = makeColumns(150);
  ASSERT_FALSE(root.hasSpace(target));
  store.seed(10, root);

  UpdateTableAction action(store, store, store, store);

  action.execute(10, target);

  EXPECT_FALSE(store.pages.at(10).containsTableId(target.tableId));
  EXPECT_EQ(store.pages.at(10).getNextPageId(), 11U);
  EXPECT_TRUE(store.pages.at(11).containsTableId(target.tableId));
  EXPECT_EQ(store.pages.at(11).get(target.tableId).columns.size(), 150U);
  EXPECT_EQ(store.writtenPageIds, (std::vector<CatalogPageId>{10, 11, 10}));
  EXPECT_FALSE(store.hasPinnedPages());

  GetTableAction getTable(store, store);
  const Table updated = getTable.execute(10, target.tableId);
  EXPECT_EQ(updated.columns.size(), 150U);
}
