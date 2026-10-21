#include "../../actions/GetTableAction.h"
#include "../../actions/UpdateTableAction.h"
#include "../CatalogActionTestSupport.h"

#include <gtest/gtest.h>

#include <string>

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

TEST(UpdateTableActionTest, PreservesOriginalTableWhenRelocationAllocationFails) {
  InMemoryCatalogPageStore store;
  CatalogPage page;
  Table filler{};
  filler.tableId = 1;
  filler.name = "filler";
  filler.columns = makeColumns(130);
  page.insert(filler);

  Table target{};
  target.tableId = 2;
  target.name = "target";
  target.columns = makeColumns(50);
  page.insert(target);
  store.seed(10, page);
  store.failNextAllocation = true;

  UpdateTableAction update(store, store, store, store);
  target.columns = makeColumns(180);
  EXPECT_THROW(update.execute(10, target), std::runtime_error);

  GetTableAction getTable(store, store);
  EXPECT_EQ(getTable.execute(10, target.tableId).columns.size(), 50U);
  EXPECT_FALSE(store.hasPinnedPages());
}
