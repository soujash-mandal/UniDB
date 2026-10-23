#include "MetadataPage.h"

#include <cstring>

MetadataPage::MetadataPage()
    : nextMetadataPageId(INVALID_PAGE_ID), firstTrackedPageId(0), bitmap{} {
  setPageOccupied(0);
}

void MetadataPage::setPageOccupied(PageId pageId) {
  if (pageId < firstTrackedPageId ||
      pageId - firstTrackedPageId >= MAX_TRACKED_PAGES) {
    throw std::out_of_range("Page ID is outside this metadata page's range");
  }
  const uint32_t relativePageId = pageId - firstTrackedPageId;
  const uint32_t byteIndex = relativePageId / BITS_PER_BYTE;
  const uint32_t bitIndex = relativePageId % BITS_PER_BYTE;
  const uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
  bitmap[byteIndex] |= mask;
}

void MetadataPage::setPageFree(PageId pageId) {
  if (pageId < firstTrackedPageId ||
      pageId - firstTrackedPageId >= MAX_TRACKED_PAGES) {
    throw std::out_of_range("Page ID is outside this metadata page's range");
  }
  const uint32_t relativePageId = pageId - firstTrackedPageId;
  const uint32_t byteIndex = relativePageId / BITS_PER_BYTE;
  const uint32_t bitIndex = relativePageId % BITS_PER_BYTE;
  const uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
  if ((bitmap[byteIndex] & mask) == 0) {
    throw std::runtime_error("Cannot free a page that is already free");
  }
  bitmap[byteIndex] &= static_cast<uint8_t>(~mask);
}

PageId MetadataPage::findFirstFreePage() const {
  for (uint32_t offset = 0; offset < MAX_TRACKED_PAGES; ++offset) {
    const uint32_t byteIndex = offset / BITS_PER_BYTE;
    const uint32_t bitIndex = offset % BITS_PER_BYTE;
    const uint8_t mask = static_cast<uint8_t>(1U << bitIndex);
    if ((bitmap[byteIndex] & mask) == 0) {
      return firstTrackedPageId + offset;
    }
  }
  return INVALID_PAGE_ID;
}
