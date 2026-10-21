#include "CatalogPageTestSupport.h"

#include <gtest/gtest.h>

TEST(CatalogPageTest, RemovingLastEntryPreservesRemainingTables) {
  CatalogPage page;
  insertThreeCatalogPageTestTables(page);

  page.remove(3);

  EXPECT_EQ(page.get(1).name, "first");
  EXPECT_EQ(page.get(2).name, "middle");
  EXPECT_FALSE(page.containsTableId(3));
}
