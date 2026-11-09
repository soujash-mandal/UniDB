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
  NUMERIC
};

struct Column {
  std::string name;
  DataType type;
  uint16_t size = 0;
  bool nullable = false;
  std::optional<std::uint32_t> numericPrecision;
  std::optional<std::int32_t> numericScale;
};

// Logical catalog metadata, independent of page layout.
struct CatalogTableDefinition {
  CatalogTableId tableId{};
  std::string name;
  std::vector<Column> columns;
};
