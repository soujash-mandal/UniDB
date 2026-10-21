#include "CatalogPageTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(CatalogPageTest, RemovingOnlyEntryLeavesPageEmptyAndReusable) {
  CatalogPage page;
  Table table = makeCatalogPageTestTable(1, "only_table");
  page.insert(table);

  page.remove(1);

  EXPECT_FALSE(page.containsTableId(1));
  EXPECT_TRUE(page.hasSpace(table));
  EXPECT_THROW(page.get(1), std::runtime_error);

  page.insert(table);
  EXPECT_EQ(page.get(1).name, "only_table");
}
