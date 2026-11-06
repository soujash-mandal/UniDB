#pragma once
#include "../domain/RdbTable.h"
#include <cstdint>
#include <vector>
class GetCatalogTablesPort {
public:
  virtual ~GetCatalogTablesPort() = default;
  virtual std::vector<RdbTableDefinition> getTables(uint32_t rootPageId) = 0;
};
