#include "MetadataPage.h"

#include <cstring>

namespace {
uint32_t BITS_PER_BYTE = 8;
uint32_t MAX_TRACKED_PAGES = BITMAP_SIZE * BITS_PER_BYTE;
} // namespace

MetadataPage::MetadataPage()
    : nextMetadataPageId(INVALID_PAGE_ID), firstTrackedPageId(0), bitmap{} {
  setPageOccupied(0);
}

void MetadataPage::setPageOccupied(PageId pageId) {
  MetadataPageId relativePageId = pageId - firstTrackedPageId;
  uint32_t byteIndex = relativePageId / BITS_PER_BYTE;
  uint32_t bitIndex = relativePageId % BITS_PER_BYTE;
  uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
  bitmap[byteIndex] |= mask;
}

PageId MetadataPage::findFirstFreePage() const {
  for (uint32_t offset = 0; offset < MAX_TRACKED_PAGES; ++offset) {
    uint32_t byteIndex = offset / BITS_PER_BYTE;
    uint32_t bitIndex = offset % BITS_PER_BYTE;
    uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
    if ((bitmap[byteIndex] & mask) == 0) {
      return firstTrackedPageId + offset;
    }
  }
  return INVALID_PAGE_ID;
}