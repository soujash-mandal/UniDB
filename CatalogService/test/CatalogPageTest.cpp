#include "../domain/CatalogPage.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <utility>

namespace {
Table makeTable(CatalogTableId id, std::string name) {
  Table table;
  table.tableId = id;
  table.name = std::move(name);
  table.firstFreeSpaceMapPageId = CATALOG_INVALID_PAGE_ID;
  return table;
}

void insertThreeTables(CatalogPage &page) {
  page.insert(makeTable(1, "first"));
  page.insert(makeTable(2, "middle"));
  page.insert(makeTable(3, "last"));
}
} // namespace

TEST(CatalogPageTest, RemovingTableReclaimsSpaceAndPreservesOtherTables) {
  CatalogPage page;
  Table largeTable = makeTable(1, std::string(5000, 'L'));
  Table smallTable = makeTable(2, "small_table");

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

TEST(CatalogPageTest, RemovingFirstEntryPreservesRemainingTables) {
  CatalogPage page;
  insertThreeTables(page);

  page.remove(1);

  EXPECT_FALSE(page.containsTableId(1));
  EXPECT_EQ(page.get(2).name, "middle");
  EXPECT_EQ(page.get(3).name, "last");
}

TEST(CatalogPageTest, RemovingMiddleEntryPreservesRemainingTables) {
  CatalogPage page;
  insertThreeTables(page);

  page.remove(2);

  EXPECT_EQ(page.get(1).name, "first");
  EXPECT_FALSE(page.containsTableId(2));
  EXPECT_EQ(page.get(3).name, "last");
}

TEST(CatalogPageTest, RemovingLastEntryPreservesRemainingTables) {
  CatalogPage page;
  insertThreeTables(page);

  page.remove(3);

  EXPECT_EQ(page.get(1).name, "first");
  EXPECT_EQ(page.get(2).name, "middle");
  EXPECT_FALSE(page.containsTableId(3));
}

TEST(CatalogPageTest, RemovingOnlyEntryLeavesPageEmptyAndReusable) {
  CatalogPage page;
  Table table = makeTable(1, "only_table");
  page.insert(table);

  page.remove(1);

  EXPECT_FALSE(page.containsTableId(1));
  EXPECT_TRUE(page.hasSpace(table));
  EXPECT_THROW(page.get(1), std::runtime_error);

  page.insert(table);
  EXPECT_EQ(page.get(1).name, "only_table");
}

TEST(CatalogPageTest, RemovingMissingTableThrowsWithoutChangingPage) {
  CatalogPage page;
  insertThreeTables(page);

  EXPECT_THROW(page.remove(99), std::runtime_error);

  EXPECT_EQ(page.get(1).name, "first");
  EXPECT_EQ(page.get(2).name, "middle");
  EXPECT_EQ(page.get(3).name, "last");
}
