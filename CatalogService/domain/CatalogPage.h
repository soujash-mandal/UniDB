#pragma once

#include "../../core/Page.h"
#include "../../core/Slot.h"
#include "CatalogPageHeader.h"

class CatalogPage {
public:
  static void Initialize(Page &page, PageId nextPageId);

  static CatalogPageHeader *GetHeader(Page &page);

  static Slot *GetSlot(Page &page, uint16_t slotId);

  static uint16_t GetFreeSpace(Page &page);
};
