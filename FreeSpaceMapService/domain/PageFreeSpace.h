#pragma once

#include <cstdint>

#include "../../core/PageId.h"

struct PageFreeSpace {
  PageId pageId;
  uint32_t freeSpace;
};
