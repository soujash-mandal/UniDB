#include "CatalogPageTestSupport.h"

#include <gtest/gtest.h>

TEST(CatalogPageTest, RepeatedGrowingUpdatesReuseCompactedPayloadSpace) {
  CatalogPage page;
  Table table = makeCatalogPageTestTable(17, "users");
  page.insert(table);

  table.name = std::string(3000, 'A');
  page.update(table);
  table.name = "users";
  page.update(table);
  table.name = std::string(3000, 'B');
  page.update(table);
  table.name = "users";
  page.update(table);
  table.name = std::string(3000, 'C');
  page.update(table);

  EXPECT_EQ(page.get(17).name, std::string(3000, 'C'));
}
