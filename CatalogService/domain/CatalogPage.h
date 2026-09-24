#pragma once

#include "../../core/Page.h"
#include "../../core/PageId.h"
#include "../../core/Slot.h"

struct CatalogPageHeader {
  PageId nextPageId;
  uint16_t slotCount;
  uint16_t freeSpaceOffset;
};

class CatalogPage {
public:
  static void Initialize(Page &page, PageId nextPageId);

  static CatalogPageHeader *GetHeader(Page &page);

  static Slot *GetSlot(Page &page, uint16_t slotId);

  static uint16_t GetFreeSpace(Page &page);
};
