#include "CatalogPageTestSupport.h"

#include <gtest/gtest.h>

TEST(CatalogPageTest, RemovingFirstEntryPreservesRemainingTables) {
  CatalogPage page;
  insertThreeCatalogPageTestTables(page);

  page.remove(1);

  EXPECT_FALSE(page.containsTableId(1));
  EXPECT_EQ(page.get(2).name, "middle");
  EXPECT_EQ(page.get(3).name, "last");
}
