#pragma once

#include "./PageId.h"
#include <cstdint>

struct PageHeader {
  PageId pageId;
  uint16_t slotCount;
  uint16_t freeSpaceOffset;
};
