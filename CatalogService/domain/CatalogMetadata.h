#pragma once

#include <cstdint>

#include "../../core/PageId.h"
#include "../../core/TableId.h"

struct CatalogMetadata {
  static constexpr PageId INVALID_PAGE_ID = UINT32_MAX;

  TableId nextTableId = 1;
  PageId firstCatalogPageId = INVALID_PAGE_ID;
};
