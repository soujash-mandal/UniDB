#include "CatalogPageTestSupport.h"

#include <gtest/gtest.h>

#include <cstring>
#include <stdexcept>

TEST(CatalogPageTest, LooksUpAndUpdatesTableMetadataByName) {
  CatalogPage page;
  Table table{};
  table.tableId = 11;
  table.name = "users";
  table.columns = {{"id", DataType::INT, 0, false},
                   {"name", DataType::VARCHAR, 64, false},
                   {"small_count", DataType::SMALLINT, 0, false},
                   {"amount", DataType::NUMERIC, 0, false, 12, 4}};
  page.insert(table);

  Table loaded = page.getByName("users");
  EXPECT_EQ(loaded.tableId, 11U);
  ASSERT_EQ(loaded.columns.size(), 4U);
  EXPECT_EQ(loaded.columns[1].name, "name");
  EXPECT_EQ(loaded.columns[2].type, DataType::SMALLINT);
  EXPECT_EQ(loaded.columns[3].type, DataType::NUMERIC);
  EXPECT_EQ(loaded.columns[3].numericPrecision, 12U);
  EXPECT_EQ(loaded.columns[3].numericScale, 4);
  page.update(loaded);

  const Table updated = page.getByName("users");
  EXPECT_EQ(updated.name, "users");
  ASSERT_EQ(updated.columns.size(), 4U);
  EXPECT_EQ(updated.columns[3].numericPrecision, 12U);
  EXPECT_EQ(updated.columns[3].numericScale, 4);
  EXPECT_THROW(page.getByName("missing"), std::runtime_error);
}

TEST(CatalogPageTest, RejectsNumericScaleWithoutPrecision) {
  CatalogPage page;
  Table table{};
  table.tableId = 12;
  table.name = "invalid_numeric";
  table.columns = {Column{"amount", DataType::NUMERIC, 0, false}};
  table.columns[0].numericScale = 2;

  EXPECT_THROW(page.insert(table), std::invalid_argument);
  EXPECT_FALSE(page.containsTableId(12));
}


TEST(CatalogPageTest, RejectsUnknownNumericMetadataExtensionVersion) {
  CatalogPage page;
  Table table{};
  table.tableId = 13;
  table.name = "unknown";
  page.insert(table);

  uint32_t payloadOffset = 0;
  std::memcpy(&payloadOffset, page.data() + sizeof(uint32_t) * 4,
              sizeof(payloadOffset));
  constexpr std::size_t versionOffset = 4 + 4 + 7 + 4 + sizeof(uint32_t);
  const uint8_t unsupportedVersion = 99;
  std::memcpy(page.data() + payloadOffset + versionOffset, &unsupportedVersion,
              sizeof(unsupportedVersion));

  EXPECT_THROW(page.get(13), std::runtime_error);
}
