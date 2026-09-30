#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using CatalogPageId = uint32_t;
using CatalogTableId = uint32_t;
using FSMPageId = uint32_t;
static constexpr uint32_t INVALID_PAGE_ID = UINT32_MAX;
enum class DataType { INT, BIGINT, FLOAT, DOUBLE, BOOLEAN, VARCHAR };

struct Column {
  std::string name;
  DataType type;
  uint16_t size = 0;
  bool nullable = false;
};

struct Table {
  CatalogTableId tableId;
  std::string name;
  std::vector<Column> columns;
  FSMPageId firstFreeSpaceMapPageId = INVALID_PAGE_ID;
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
  bool containsTableName(std::string name);
  void remove(CatalogTableId tableId);
  bool hasSpace(Table table);

private:
  char bytes[PAGE_SIZE]{};
};
