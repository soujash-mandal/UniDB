#include "CatalogPageTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(CatalogPageTest, RemovingMissingTableThrowsWithoutChangingPage) {
  CatalogPage page;
  insertThreeCatalogPageTestTables(page);

  EXPECT_THROW(page.remove(99), std::runtime_error);

  EXPECT_EQ(page.get(1).name, "first");
  EXPECT_EQ(page.get(2).name, "middle");
  EXPECT_EQ(page.get(3).name, "last");
}
