#pragma once

#include "../domain/RdbTable.h"

#include <cstdint>
#include <string>

class GetCatalogTableByNamePort {
public:
  virtual ~GetCatalogTableByNamePort() = default;

  virtual RdbTableDefinition getTableByName(uint32_t catalogRootPageId,
                                            const std::string &tableName) = 0;
};
