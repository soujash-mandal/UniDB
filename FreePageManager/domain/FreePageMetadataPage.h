#pragma once

#include <cstdint>

#include "FreePageMetadataPageId.h"

class FreePageMetadata {
public:
  static constexpr FreePageMetadataPageId INVALID_PAGE_ID = UINT32_MAX;
  static constexpr uint32_t FIRST_TRACKED_PAGE_ID_OFFSET = sizeof(FreePageMetadataPageId);
  static constexpr uint32_t BITMAP_OFFSET = sizeof(FreePageMetadataPageId) + sizeof(FreePageMetadataPageId);
  static constexpr uint32_t BITMAP_SIZE = Page::PAGE_SIZE - BITMAP_OFFSET;
  static constexpr uint32_t BITS_PER_BYTE = 8;
  static constexpr uint32_t MAX_TRACKED_PAGES = BITMAP_SIZE * BITS_PER_BYTE;

private:
  FreePageMetadataPageId nextMetadataPageId;
  FreePageMetadataPageId firstTrackedPageId;
  uint8_t bitmap[BITMAP_SIZE];

public:
  FreePageMetadata();
  FreePageMetadataPageId getNextMetadataPageId() const;
  void setNextMetadataPageId(FreePageMetadataPageId pageId);
  FreePageMetadataPageId getFirstTrackedPageId() const;
  void setFirstTrackedPageId(FreePageMetadataPageId pageId);
  bool isPageOccupied(FreePageMetadataPageId pageId) const;
  void setPageOccupied(FreePageMetadataPageId pageId, bool occupied);
  FreePageMetadataPageId findFirstFreePage() const;
  void readFromPage(const Page &page);
  void writeToPage(Page &page) const;
};
