#pragma once

#include "CatalogTypes.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using FSMPageId = uint32_t;

struct Table {
  CatalogTableId tableId;
  std::string name;
  std::vector<Column> columns;
  FSMPageId firstFreeSpaceMapPageId = CATALOG_INVALID_PAGE_ID;
};

namespace {
struct Header {
  CatalogPageId nextPageId;
  uint32_t tableCount;
  uint32_t freeSpaceOffset;
};
struct Entry {
  CatalogTableId tableId;
  uint32_t offset;
  uint32_t size;
};
static constexpr uint32_t HEADER_SIZE = sizeof(Header);
static constexpr uint32_t ENTRY_SIZE = sizeof(Entry);
} // namespace

class CatalogPage {
public:
  static constexpr std::size_t PAGE_SIZE = 8192;
  CatalogPage();
  char *data() { return bytes; }
  const char *data() const { return bytes; }
  CatalogPageId getNextPageId();
  void setNextPageId(CatalogPageId pageId);
  void insert(Table table);
  Table get(CatalogTableId tableId);
  Table getByName(const std::string &name);
  void update(Table table);
  bool containsTableName(std::string name);
  bool containsTableId(CatalogTableId tableId);
  void remove(CatalogTableId tableId);
  bool hasSpace(Table table);

private:
  char bytes[PAGE_SIZE]{};
};
