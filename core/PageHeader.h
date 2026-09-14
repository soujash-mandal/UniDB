#pragma once

#include <cstdint>

struct PageHeader {
  uint32_t pageId;
  uint16_t slotCount;
  uint16_t freeSpaceOffset;
};
