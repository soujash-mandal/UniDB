#pragma once

#include <cstdint>

struct PageHeader {
  uint16_t slotCount;
  uint16_t freeSpaceStart;
  uint16_t freeSpaceEnd;
};
