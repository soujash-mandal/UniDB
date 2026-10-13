#pragma once

#include <cstdint>
#include <string>
#include <vector>

using CatalogPageId = uint32_t;
using CatalogTableId = uint32_t;
static constexpr uint32_t CATALOG_INVALID_PAGE_ID = UINT32_MAX;

enum class DataType { INT, BIGINT, FLOAT, DOUBLE, BOOLEAN, VARCHAR };

struct Column {
  std::string name;
  DataType type;
  uint16_t size = 0;
  bool nullable = false;
};

// Logical catalog metadata, independent of page layout.
struct CatalogTableDefinition {
  CatalogTableId tableId{};
  std::string name;
  std::vector<Column> columns;
  // Opaque root locator for TupleService; no CatalogPage/FSM layout is exposed.
  CatalogPageId tupleRootPageId{CATALOG_INVALID_PAGE_ID};
};
