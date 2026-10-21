#include "CatalogPageTestSupport.h"

#include <gtest/gtest.h>

TEST(CatalogPageTest, RemovingTableReclaimsSpaceAndPreservesOtherTables) {
  CatalogPage page;
  Table largeTable = makeCatalogPageTestTable(1, std::string(5000, 'L'));
  Table smallTable = makeCatalogPageTestTable(2, "small_table");

  ASSERT_TRUE(page.hasSpace(largeTable));
  page.insert(largeTable);
  page.insert(smallTable);
  EXPECT_FALSE(page.hasSpace(largeTable));

  page.remove(1);

  EXPECT_FALSE(page.containsTableId(1));
  EXPECT_TRUE(page.containsTableId(2));
  EXPECT_EQ(page.get(2).name, "small_table");
  EXPECT_TRUE(page.hasSpace(largeTable));

  page.insert(largeTable);
  EXPECT_EQ(page.get(1).name, std::string(5000, 'L'));
  EXPECT_EQ(page.get(2).name, "small_table");
}
