#pragma once

#include "../../actions/GetTableDefinitionAction.h"

#include <stdexcept>
#include <string>

class FakeDefinitionRootPagePort final : public GetCatalogRootPageIdPort {
public:
  uint32_t rootPageId = 17;
  uint32_t calls = 0;
  uint32_t receivedMetadataPageId = 0;

  uint32_t getCatalogRootPageId(uint32_t metadataPageId) override {
    ++calls;
    receivedMetadataPageId = metadataPageId;
    return rootPageId;
  }
};

class FakeDefinitionTableLookupPort final : public GetCatalogTableByNamePort {
public:
  uint32_t calls = 0;
  uint32_t receivedRootPageId = 0;
  std::string receivedTableName;
  RdbTableDefinition result;
  bool shouldThrow = false;

  RdbTableDefinition getTableByName(uint32_t catalogRootPageId,
                                   const std::string &tableName) override {
    ++calls;
    receivedRootPageId = catalogRootPageId;
    receivedTableName = tableName;
    if (shouldThrow) {
      throw std::runtime_error("table lookup failed");
    }
    return result;
  }
};

inline RdbTableDefinition validDefinitionForLookup() {
  RdbTableDefinition table;
  table.tableId = 42;
  table.name = "events";
  table.columns = {RdbColumn{"event_id", RdbDataType::INT, 4, false},
                   RdbColumn{"description", RdbDataType::VARCHAR, 120, true}};
  return table;
}
