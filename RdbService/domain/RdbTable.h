#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// RDBService-owned catalog model. Adapters translate this model to and from
// CatalogService types so RDBService's public ports do not depend on another
// service's domain objects.
// Append new values only: the numeric value is persisted in catalog pages.
enum class RdbDataType {
  INT,
  BIGINT,
  FLOAT,
  DOUBLE,
  BOOLEAN,
  VARCHAR,
  SMALLINT,
  NUMERIC,
  CHAR,
  TEXT
};

struct RdbColumn {
  std::string name;
  RdbDataType type;
  uint16_t size = 0;
  bool nullable = false;
  std::optional<std::uint32_t> numericPrecision;
  std::optional<std::int32_t> numericScale;
  std::optional<std::uint32_t> characterLength;
};

struct RdbTableDefinition {
  uint32_t tableId = 0;
  std::string name;
  std::vector<RdbColumn> columns;
  uint32_t firstSpaceMapPageId = UINT32_MAX;
};
