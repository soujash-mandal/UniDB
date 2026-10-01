#pragma once

#include <cstddef>
#include <cstdint>

using CatalogPageId = uint32_t;
using CatalogTableId = uint32_t;

class RdbMetadataPage {
public:
  static constexpr std::size_t PAGE_SIZE = 8192;
  static constexpr CatalogPageId INVALID_PAGE_ID = UINT32_MAX;

  RdbMetadataPage();

  char *data() { return bytes; }
  const char *data() const { return bytes; }

  CatalogTableId getNextTableId();
  void setNextTableId(CatalogTableId tableId);

  CatalogPageId getCatalogRootPageId();
  void setCatalogRootPageId(CatalogPageId pageId);

private:
  char bytes[PAGE_SIZE]{};
};
