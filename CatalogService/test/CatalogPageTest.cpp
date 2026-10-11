#include "../domain/CatalogPage.h"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(CatalogPageTest, LooksUpAndUpdatesTableMetadataByName) {
  CatalogPage page;
  Table table{};
  table.tableId = 11;
  table.name = "users";
  table.columns = {{"id", DataType::INT, 0, false},
                   {"name", DataType::VARCHAR, 64, false}};

  page.insert(table);
  Table loaded = page.getByName("users");
  EXPECT_EQ(loaded.tableId, 11U);
  ASSERT_EQ(loaded.columns.size(), 2U);
  EXPECT_EQ(loaded.columns[1].name, "name");

  loaded.firstTuplePageId = 42;
  page.update(loaded);
  EXPECT_EQ(page.getByName("users").firstTuplePageId, 42U);
  EXPECT_THROW(page.getByName("missing"), std::runtime_error);
}
