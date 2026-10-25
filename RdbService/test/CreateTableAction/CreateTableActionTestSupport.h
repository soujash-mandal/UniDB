#pragma once

#include "../../actions/CreateTableAction.h"

#include <stdexcept>
#include <string>
#include <vector>

class FakeGetCatalogRootPageIdPort final : public GetCatalogRootPageIdPort {
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

class FakeAllocateNextTableIdPort final : public AllocateNextTableIdPort {
public:
  uint32_t tableId = 42;
  uint32_t calls = 0;
  uint32_t receivedMetadataPageId = 0;

  uint32_t allocateNextTableId(uint32_t metadataPageId) override {
    ++calls;
    receivedMetadataPageId = metadataPageId;
    return tableId;
  }
};

class FakeCreateCatalogTablePort final : public CreateCatalogTablePort {
public:
  uint32_t calls = 0;
  uint32_t receivedRootPageId = 0;
  uint32_t receivedTableId = 0;
  RdbTableDefinition receivedTable;
  bool shouldThrow = false;

  uint32_t createTable(uint32_t catalogRootPageId, uint32_t tableId,
                       const RdbTableDefinition &table) override {
    ++calls;
    receivedRootPageId = catalogRootPageId;
    receivedTableId = tableId;
    receivedTable = table;
    if (shouldThrow) {
      throw std::runtime_error("catalog create failed");
    }
    return tableId;
  }
};

inline RdbTableDefinition validTableDefinition() {
  RdbTableDefinition table;
  table.name = "events";
  table.columns = {RdbColumn{"event_id", RdbDataType::INT, 4, false},
                   RdbColumn{"description", RdbDataType::VARCHAR, 120, true}};
  return table;
}
