#include "CatalogPageTestSupport.h"

#include <gtest/gtest.h>

TEST(CatalogPageTest, RemovingMiddleEntryPreservesRemainingTables) {
  CatalogPage page;
  insertThreeCatalogPageTestTables(page);

  page.remove(2);

  EXPECT_EQ(page.get(1).name, "first");
  EXPECT_FALSE(page.containsTableId(2));
  EXPECT_EQ(page.get(3).name, "last");
}
