#pragma once

#include "../domain/RdbTable.h"

#include <cstdint>

class CreateCatalogTablePort {
public:
  virtual ~CreateCatalogTablePort() = default;

  virtual uint32_t createTable(uint32_t catalogRootPageId, uint32_t tableId,
                               const RdbTableDefinition &table) = 0;
};
