#pragma once

#include "../../actions/GetTableDefinitionAction.h"

#include <stdexcept>
#include <string>

class GetTableDefinitionFakeRootPort final : public GetCatalogRootPageIdPort {
public:
  uint32_t result = 17;
  uint32_t calls = 0;
  uint32_t receivedMetadataPageId = 0;

  uint32_t getCatalogRootPageId(uint32_t metadataPageId) override {
    ++calls;
    receivedMetadataPageId = metadataPageId;
    return result;
  }
};

class GetTableDefinitionFakeCatalogPort final
    : public GetCatalogTableByNamePort {
public:
  RdbTableDefinition result{42,
                            "events",
                            {{"event_id", RdbDataType::INT, 4, false},
                             {"description", RdbDataType::VARCHAR, 120, true}},
                            91};
  uint32_t calls = 0;
  uint32_t receivedCatalogRootPageId = 0;
  std::string receivedTableName;
  bool shouldThrow = false;

  RdbTableDefinition getTableByName(uint32_t catalogRootPageId,
                                    const std::string &tableName) override {
    ++calls;
    receivedCatalogRootPageId = catalogRootPageId;
    receivedTableName = tableName;
    if (shouldThrow) {
      throw std::runtime_error("catalog lookup failed");
    }
    return result;
  }
};
