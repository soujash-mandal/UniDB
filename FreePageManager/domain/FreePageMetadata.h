#pragma once

#include "../../core/Page.h"
#include "../../core/PageId.h"

#include <cstdint>

class FreePageMetadata {
public:
  static constexpr PageId INVALID_PAGE_ID = UINT32_MAX;
  static constexpr uint32_t BITMAP_OFFSET = sizeof(PageId);
  static constexpr uint32_t BITMAP_SIZE = Page::PAGE_SIZE - BITMAP_OFFSET;
  static constexpr uint32_t BITS_PER_BYTE = 8;
  static constexpr uint32_t MAX_TRACKED_PAGES = BITMAP_SIZE * BITS_PER_BYTE;

private:
  PageId nextMetadataPageId;
  uint8_t bitmap[BITMAP_SIZE];

public:
  FreePageMetadata();
  PageId getNextMetadataPageId() const;
  void setNextMetadataPageId(PageId pageId);
  bool isPageOccupied(PageId pageId) const;
  void setPageOccupied(PageId pageId, bool occupied);
  PageId findFirstFreePage() const;
  void readFromPage(const Page &page);
  void writeToPage(Page &page) const;
};
