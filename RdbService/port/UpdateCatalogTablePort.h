#pragma once

#include "../domain/RdbTable.h"

#include <cstdint>

class UpdateCatalogTablePort {
public:
  virtual ~UpdateCatalogTablePort() = default;

  virtual void updateTable(uint32_t catalogRootPageId,
                           const RdbTableDefinition &table,
                           uint32_t firstSpaceMapPageId) = 0;
};
