#pragma once

#include "../domain/RdbTable.h"

#include <string>

class CatalogTablePort {
public:
  virtual ~CatalogTablePort() = default;

  virtual uint32_t createTable(uint32_t catalogRootPageId, uint32_t tableId,
                               const RdbTableDefinition &table) = 0;
  virtual RdbTableDefinition getTableByName(uint32_t catalogRootPageId,
                                            const std::string &tableName) = 0;
  virtual void updateTable(uint32_t catalogRootPageId,
                           const RdbTableDefinition &table,
                           uint32_t firstSpaceMapPageId) = 0;
};
