#pragma once

#include "../domain/RdbTable.h"
#include "../port/CatalogTablePort.h"
#include "../port/RdbMetadataPort.h"

#include <stdexcept>
#include <string>
#include <vector>

class FakeRdbMetadataPort final : public RdbMetadataPort {
public:
  CatalogPageId getCatalogRootPageId() override {
    ++rootLookups;
    if (failRootLookup) {
      throw std::runtime_error("Injected metadata lookup failure");
    }
    return catalogRootPageId;
  }

  uint32_t allocateNextTableId() override {
    ++tableIdAllocations;
    if (failTableIdAllocation) {
      throw std::runtime_error("Injected table ID allocation failure");
    }
    return nextTableId++;
  }

  CatalogPageId catalogRootPageId = 10;
  uint32_t nextTableId = 1;
  uint32_t rootLookups = 0;
  uint32_t tableIdAllocations = 0;
  bool failRootLookup = false;
  bool failTableIdAllocation = false;
};

class FakeCatalogTablePort final : public CatalogTablePort {
public:
  uint32_t createTable(uint32_t rootPageId, uint32_t tableId,
                       const RdbTableDefinition &table) override {
    lastRootPageId = rootPageId;
    lastTableId = tableId;
    lastCreatedTable = table;
    ++createCalls;
    if (failCreate) {
      throw std::runtime_error("Injected table creation failure");
    }
    return tableId;
  }

  RdbTableDefinition getTableByName(uint32_t rootPageId,
                                    const std::string &tableName) override {
    lastRootPageId = rootPageId;
    lastLookupName = tableName;
    ++lookupCalls;
    if (failLookup) {
      throw std::runtime_error("Injected table lookup failure");
    }
    if (storedTable.name != tableName) {
      throw std::runtime_error("Table not found");
    }
    return storedTable;
  }

  void updateTable(uint32_t, const RdbTableDefinition &) override {}

  uint32_t lastRootPageId = 0;
  uint32_t lastTableId = 0;
  uint32_t createCalls = 0;
  uint32_t lookupCalls = 0;
  std::string lastLookupName;
  RdbTableDefinition lastCreatedTable;
  RdbTableDefinition storedTable;
  bool failCreate = false;
  bool failLookup = false;
};
