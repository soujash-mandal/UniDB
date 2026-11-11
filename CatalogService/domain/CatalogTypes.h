#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using CatalogPageId = uint32_t;
using CatalogTableId = uint32_t;
static constexpr uint32_t CATALOG_INVALID_PAGE_ID = UINT32_MAX;

// Append new values only: the numeric value is persisted in catalog pages.
enum class DataType {
  INT,
  BIGINT,
  FLOAT,
  DOUBLE,
  BOOLEAN,
  VARCHAR,
  SMALLINT,
  NUMERIC,
  CHAR,
  TEXT,
  DATE,
  TIME,
  TIME_WITH_TIME_ZONE,
  TIMESTAMP,
  TIMESTAMP_WITH_TIME_ZONE,
  INTERVAL
};

struct Column {
  std::string name;
  DataType type;
  uint16_t size = 0;
  bool nullable = false;
  std::optional<std::uint32_t> numericPrecision = std::nullopt;
  std::optional<std::int32_t> numericScale = std::nullopt;
  std::optional<std::uint32_t> characterLength = std::nullopt;
  std::optional<std::uint32_t> temporalPrecision = std::nullopt;
};

// Logical catalog metadata, independent of page layout.
struct CatalogTableDefinition {
  CatalogTableId tableId{};
  std::string name;
  std::vector<Column> columns;
};
