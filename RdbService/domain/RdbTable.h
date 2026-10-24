#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

// RDBService-owned catalog model. Adapters translate this model to and from
// CatalogService types so RDBService's public ports do not depend on another
// service's domain objects.
enum class RdbDataType { INT, BIGINT, FLOAT, DOUBLE, BOOLEAN, VARCHAR };

struct RdbColumn {
  std::string name;
  RdbDataType type;
  uint16_t size = 0;
  bool nullable = false;
};

struct RdbTableDefinition {
  uint32_t tableId = 0;
  std::string name;
  std::vector<RdbColumn> columns;
  uint32_t firstSpaceMapPageId = UINT32_MAX;
};

using RdbValue = std::variant<int64_t, std::string>;
using RdbRow = std::vector<RdbValue>;
