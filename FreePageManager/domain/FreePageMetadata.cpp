#include "FreePageMetadata.h"

#include <cstring>

FreePageMetadata::FreePageMetadata()
    : nextMetadataPageId(INVALID_PAGE_ID), bitmap{} {}

PageId FreePageMetadata::getNextMetadataPageId() const {
  return nextMetadataPageId;
}

void FreePageMetadata::setNextMetadataPageId(PageId pageId) {
  nextMetadataPageId = pageId;
}

bool FreePageMetadata::isPageOccupied(PageId pageId) const {
  uint32_t byteIndex = pageId / BITS_PER_BYTE;
  uint32_t bitIndex = pageId % BITS_PER_BYTE;
  uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
  return (bitmap[byteIndex] & mask) != 0;
}

void FreePageMetadata::setPageOccupied(PageId pageId, bool occupied) {
  uint32_t byteIndex = pageId / BITS_PER_BYTE;
  uint32_t bitIndex = pageId % BITS_PER_BYTE;
  uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
  if (occupied) {
    bitmap[byteIndex] |= mask;
  } else {
    bitmap[byteIndex] &= static_cast<uint8_t>(~mask);
  }
}

PageId FreePageMetadata::findFirstFreePage() const {
  for (PageId pageId = 0; pageId < MAX_TRACKED_PAGES; ++pageId) {
    if (!isPageOccupied(pageId)) {
      return pageId;
    }
  }
  return INVALID_PAGE_ID;
}

void FreePageMetadata::readFromPage(const Page &page) {
  std::memcpy(&nextMetadataPageId, page.data(), sizeof(PageId));
  std::memcpy(bitmap, page.data() + BITMAP_OFFSET, BITMAP_SIZE);
}

void FreePageMetadata::writeToPage(Page &page) const {
  std::memcpy(page.data(), &nextMetadataPageId, sizeof(PageId));
  std::memcpy(page.data() + BITMAP_OFFSET, bitmap, BITMAP_SIZE);
}
