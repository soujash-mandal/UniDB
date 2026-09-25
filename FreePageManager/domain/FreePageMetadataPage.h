#pragma once

#include "FreePageMetadataPageId.h"

#include <cstdint>

class FreePageMetadataPage {
public:
  static constexpr uint32_t BITS_PER_BYTE = 8;
  static constexpr uint32_t PAGE_SIZE = 8192;
  static constexpr FreePageMetadataPageId INVALID_PAGE_ID = UINT32_MAX;
  static constexpr uint32_t FIRST_TRACKED_PAGE_ID_OFFSET =
      sizeof(FreePageMetadataPageId);
  static constexpr uint32_t BITMAP_OFFSET =
      sizeof(FreePageMetadataPageId) + sizeof(FreePageMetadataPageId);
  static constexpr uint32_t BITMAP_SIZE = PAGE_SIZE - BITMAP_OFFSET;
  static constexpr uint32_t MAX_TRACKED_PAGES = BITMAP_SIZE * BITS_PER_BYTE;

private:
  FreePageMetadataPageId nextMetadataPageId;
  FreePageMetadataPageId firstTrackedPageId;
  uint8_t bitmap[BITMAP_SIZE];

public:
  FreePageMetadataPage();
  FreePageMetadataPageId getNextMetadataPageId() const;
  void setNextMetadataPageId(FreePageMetadataPageId pageId);
  FreePageMetadataPageId getFirstTrackedPageId() const;
  void setFirstTrackedPageId(FreePageMetadataPageId pageId);
  bool isPageOccupied(FreePageMetadataPageId pageId) const;
  void setPageOccupied(FreePageMetadataPageId pageId, bool occupied);
  FreePageMetadataPageId findFirstFreePage() const;
  char *data() { return reinterpret_cast<char *>(this); }
  const char *data() const { return reinterpret_cast<const char *>(this); }
};
