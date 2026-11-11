#include "CatalogPageTestSupport.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <stdexcept>

TEST(CatalogPage, RoundTripsDateTimeTypeIdentityAndTemporalPrecision) {
  CatalogPage page;
  Table table = makeCatalogPageTestTable(42, "datetime_types");
  table.columns = {
      Column{"date_value", DataType::DATE},
      Column{"time_value", DataType::TIME, 0, false, std::nullopt,
             std::nullopt, std::nullopt, std::uint32_t{0}},
      Column{"timetz_value", DataType::TIME_WITH_TIME_ZONE, 0, false,
             std::nullopt, std::nullopt, std::nullopt, std::uint32_t{3}},
      Column{"timestamp_value", DataType::TIMESTAMP, 0, false,
             std::nullopt, std::nullopt, std::nullopt, std::uint32_t{6}},
      Column{"timestamptz_value", DataType::TIMESTAMP_WITH_TIME_ZONE, 0, false,
             std::nullopt, std::nullopt, std::nullopt, std::uint32_t{2}},
      Column{"interval_value", DataType::INTERVAL},
  };

  page.insert(table);
  const Table restored = page.get(42);

  ASSERT_EQ(restored.columns.size(), 6);
  EXPECT_EQ(restored.columns[0].type, DataType::DATE);
  EXPECT_EQ(restored.columns[1].type, DataType::TIME);
  EXPECT_EQ(restored.columns[1].temporalPrecision, 0);
  EXPECT_EQ(restored.columns[2].type, DataType::TIME_WITH_TIME_ZONE);
  EXPECT_EQ(restored.columns[2].temporalPrecision, 3);
  EXPECT_EQ(restored.columns[3].type, DataType::TIMESTAMP);
  EXPECT_EQ(restored.columns[3].temporalPrecision, 6);
  EXPECT_EQ(restored.columns[4].type, DataType::TIMESTAMP_WITH_TIME_ZONE);
  EXPECT_EQ(restored.columns[4].temporalPrecision, 2);
  EXPECT_EQ(restored.columns[5].type, DataType::INTERVAL);
  EXPECT_FALSE(restored.columns[5].temporalPrecision.has_value());
}

TEST(CatalogPage, RejectsTemporalPrecisionOnNonTemporalTypes) {
  CatalogPage page;
  Table table = makeCatalogPageTestTable(43, "invalid_datetime_modifier");
  table.columns = {
      Column{"date_value", DataType::DATE, 0, false, std::nullopt,
             std::nullopt, std::nullopt, std::uint32_t{3}},
  };

  EXPECT_THROW(page.insert(table), std::invalid_argument);
}

TEST(CatalogPage, RejectsTemporalPrecisionAbovePostgresMaximum) {
  CatalogPage page;
  Table table = makeCatalogPageTestTable(44, "invalid_datetime_precision");
  table.columns = {
      Column{"time_value", DataType::TIME, 0, false, std::nullopt,
             std::nullopt, std::nullopt, std::uint32_t{7}},
  };

  EXPECT_THROW(page.insert(table), std::invalid_argument);
}
