#pragma once

#include "../../core/PageId.h"
#include <cstdint>

struct CatalogPageHeader {
  PageId nextPageId;
  uint16_t slotCount;
  uint16_t freeSpaceOffset;
};
