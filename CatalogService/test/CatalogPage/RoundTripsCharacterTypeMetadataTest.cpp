#include "CatalogPageTestSupport.h"

#include <gtest/gtest.h>

TEST(CatalogPage, RoundTripsCharacterTypeIdentityAndLengthModifiers) {
  CatalogPage page;
  Table table = makeCatalogPageTestTable(41, "character_types");
  table.columns = {
      Column{"fixed", DataType::CHAR, 0, false, std::nullopt, std::nullopt, 5},
      Column{"bounded", DataType::VARCHAR, 0, false, std::nullopt, std::nullopt, 32},
      Column{"unbounded", DataType::VARCHAR, 0, false, std::nullopt, std::nullopt, std::nullopt},
      Column{"body", DataType::TEXT, 0, false, std::nullopt, std::nullopt, std::nullopt},
  };

  page.insert(table);
  const Table restored = page.get(41);

  ASSERT_EQ(restored.columns.size(), 4);
  EXPECT_EQ(restored.columns[0].type, DataType::CHAR);
  EXPECT_EQ(restored.columns[0].characterLength, 5);
  EXPECT_EQ(restored.columns[1].type, DataType::VARCHAR);
  EXPECT_EQ(restored.columns[1].characterLength, 32);
  EXPECT_EQ(restored.columns[2].type, DataType::VARCHAR);
  EXPECT_FALSE(restored.columns[2].characterLength.has_value());
  EXPECT_EQ(restored.columns[3].type, DataType::TEXT);
  EXPECT_FALSE(restored.columns[3].characterLength.has_value());
}
