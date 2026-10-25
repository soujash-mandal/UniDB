#pragma once

#include "../../actions/SelectAllRowsAction.h"
#include "../../domain/RdbTable.h"

#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class FakeSelectRootPagePort final : public GetCatalogRootPageIdPort {
public:
  uint32_t calls = 0;
  uint32_t metadataPageId = 0;
  uint32_t rootPageId = 17;
  bool shouldThrow = false;

  uint32_t getCatalogRootPageId(uint32_t requestedMetadataPageId) override {
    ++calls;
    metadataPageId = requestedMetadataPageId;
    if (shouldThrow) {
      throw std::runtime_error("metadata lookup failed");
    }
    return rootPageId;
  }
};

class FakeSelectTableLookupPort final : public GetCatalogTableByNamePort {
public:
  uint32_t calls = 0;
  uint32_t catalogRootPageId = 0;
  std::string tableName;
  RdbTableDefinition table;
  bool shouldThrow = false;

  RdbTableDefinition getTableByName(
      uint32_t requestedRootPageId,
      const std::string &requestedTableName) override {
    ++calls;
    catalogRootPageId = requestedRootPageId;
    tableName = requestedTableName;
    if (shouldThrow) {
      throw std::runtime_error("table lookup failed");
    }
    return table;
  }
};

class FakeSelectTablePagesPort final : public GetTablePagesPort {
public:
  uint32_t calls = 0;
  uint32_t rootSpaceMapPageId = 0;
  std::vector<uint32_t> pageIds;
  bool shouldThrow = false;

  std::vector<uint32_t> getTablePages(uint32_t rootPageId) override {
    ++calls;
    rootSpaceMapPageId = rootPageId;
    if (shouldThrow) {
      throw std::runtime_error("page enumeration failed");
    }
    return pageIds;
  }
};

class FakeSelectAllTuplesPort final : public GetAllTuplesPort {
public:
  uint32_t calls = 0;
  std::vector<uint32_t> requestedPageIds;
  std::vector<std::vector<char>> getAllTuples(uint32_t pageId) override {
    ++calls;
    requestedPageIds.push_back(pageId);
    const auto found = rowsByPage.find(pageId);
    if (found == rowsByPage.end()) {
      return {};
    }
    return found->second;
  }

  std::map<uint32_t, std::vector<std::vector<char>>> rowsByPage;
};

inline void setRows(FakeSelectAllTuplesPort &port, uint32_t pageId,
                    std::vector<std::vector<char>> rows) {
  port.rowsByPage.emplace(pageId, std::move(rows));
}
